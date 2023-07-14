//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "hermes/config_server.h"
#include "hermes_mdm/hermes_mdm.h"
#include "hermes_dpe/hermes_dpe.h"
#include "bdev/bdev.h"

namespace hermes::mdm {

/** Type name simplification for the various map types */
typedef std::unordered_map<hshm::charbuf, BlobId> BLOB_ID_MAP_T;
typedef std::unordered_map<hshm::charbuf, TagId> TAG_ID_MAP_T;
typedef std::unordered_map<BlobId, BlobInfo> BLOB_MAP_T;
typedef std::unordered_map<TagId, TagInfo> TAG_MAP_T;
typedef hipc::mpsc_queue<IoStat> IO_PATTERN_LOG_T;

class Server : public TaskLib {
 public:
  /**====================================
   * Configuration
   * ===================================*/
   ServerConfig server_config_;
   u32 node_id_;

  /**====================================
   * Maps
   * ===================================*/
  BLOB_ID_MAP_T blob_id_map_;
  TAG_ID_MAP_T tag_id_map_;
  BLOB_MAP_T blob_map_;
  TAG_MAP_T tag_map_;
  std::atomic<u64> id_alloc_;

  /**====================================
  * I/O pattern log
  * ===================================*/
  IO_PATTERN_LOG_T *io_pattern_log_;
  bool enable_io_tracing_;
  bool is_mpi_;

  /**====================================
   * Targets + devices
   * ===================================*/
  std::vector<bdev::Client> targets_;

 public:
  Server() = default;

  void Run(MultiQueue *queue, u32 method, Task *task) override {
    switch (method) {
      case Method::kConstruct: {
        Construct(queue, reinterpret_cast<ConstructTask *>(task));
        break;
      }
      case Method::kDestruct: {
        Destruct(queue, reinterpret_cast<DestructTask *>(task));
        break;
      }
      case Method::kGetOrCreateTag: {
        GetOrCreateTag(queue, reinterpret_cast<GetOrCreateTagTask *>(task));
        break;
      }
    }
  }

  void Construct(MultiQueue *queue, ConstructTask *task) {
    id_alloc_ = 0;
    hipc::string &config_path = task->server_config_path_.get_ref();

    // Load hermes config
    if (config_path.size() == 0) {
      config_path = GetEnvSafe(Constant::kHermesServerConf);
    }
    HILOG(kInfo, "Loading server configuration: {}", config_path.str())
    server_config_.LoadFromFile(config_path.str());
    node_id_ = 0;  // TODO(llogan)

    // Parse targets
    for (DeviceInfo &dev : server_config_.devices_) {
      bdev::Client client;
      std::string dev_type;
      if (dev.mount_point_.empty()) {
        dev_type = "posix_bdev";
      } else {
        dev_type = "ram_bdev";
      }
      client.Create("hermes_" + dev.dev_name_,
                    dev_type,
                    DomainId::GetLocal(),
                    dev);
      targets_.emplace_back(std::move(client));
    }
    targets_.emplace_back();

    // Create targets
    task->SetComplete();
  }

  void Destruct(MultiQueue *queue, DestructTask *task) {
    task->SetComplete();
  }

  /**====================================
   * Tag Operations
   * ===================================*/

  /** Get or create a tag */
  void GetOrCreateTag(MultiQueue *queue, GetOrCreateTagTask *task) {
    TagId tag_id;

    // Check if the tag exists
    hshm::charbuf tag_name = hshm::to_charbuf(*task->tag_name_);
    bool did_create;
    if (tag_name.size() == 0) {
      did_create = tag_id_map_.find(tag_name) == tag_id_map_.end();
    }

    // Emplace bucket if it does not already exist
    if (did_create) {
      tag_id.unique_ = id_alloc_.fetch_add(1);
      tag_id.node_id_ = LABSTOR_RUNTIME->rpc_.node_id_;
      HILOG(kDebug, "Creating tag for the first time: {} {}", tag_name, tag_id)
      tag_id_map_.emplace(tag_name, tag_id);
      tag_map_.emplace(tag_id, TagInfo());
      TagInfo &info = tag_map_[tag_id];
      info.name_ = tag_name;
      info.tag_id_ = tag_id;
      info.owner_ = task->blob_owner_;
      info.internal_size_ = task->backend_size_;
    } else {
      if (tag_name.size()) {
        HILOG(kDebug, "Found existing tag: {}", tag_name)
        tag_id = tag_id_map_[tag_name];
      } else {
        HILOG(kDebug, "Found existing tag: {}", task->tag_id_)
        tag_id = task->tag_id_;
      }
      TagInfo &info = tag_map_[tag_id];
    }

    task->tag_id_ = tag_id;
    // task->did_create_ = did_create;
    task->SetComplete();
  }

  /** Get tag ID */
  void GetTagId(MultiQueue *queue, GetTagIdTask *task) {
    hshm::charbuf tag_name = hshm::to_charbuf(*task->tag_name_);
    auto it = tag_id_map_.find(tag_name);
    if (it == tag_id_map_.end()) {
      task->tag_id_ = TagId::GetNull();
      task->SetComplete();
      return;
    }
    task->tag_id_ = it->second;
    task->SetComplete();
  }

  /** Get tag name */
  void GetTagName(MultiQueue *queue, GetTagNameTask *task) {
    auto it = tag_map_.find(task->tag_id_);
    if (it == tag_map_.end()) {
      task->SetComplete();
      return;
    }
    (*task->tag_name_) = it->second.name_;
    task->SetComplete();
  }

  /** Rename tag */
  void RenameTag(MultiQueue *queue, RenameTagTask *task) {
    auto it = tag_map_.find(task->tag_id_);
    if (it == tag_map_.end()) {
      task->SetComplete();
      return;
    }
    (*task->tag_name_) = (*task->tag_name_);
    task->SetComplete();
  }

  /** Destroy tag */
  void DestroyTag(MultiQueue *queue, DestroyTagTask *task) {
    auto it = tag_map_.find(task->tag_id_);
    if (it == tag_map_.end()) {
      task->SetComplete();
      return;
    }
    tag_id_map_.erase(it->second.name_);
    tag_map_.erase(it);
    task->SetComplete();
  }

  /** Add a blob to a tag */
  void TagAddBlob(MultiQueue *queue, TagAddBlobTask *task) {
    auto it = tag_map_.find(task->tag_id_);
    if (it == tag_map_.end()) {
      task->SetComplete();
      return;
    }
    TagInfo &tag = it->second;
    return task->SetComplete();
  }

  /** Remove a blob from a tag */
  void TagRemoveBlob(MultiQueue *queue, TagRemoveBlobTask *task) {
    auto it = tag_map_.find(task->tag_id_);
    if (it == tag_map_.end()) {
      task->SetComplete();
      return;
    }
    TagInfo &tag = it->second;
    auto blob_it = std::find(tag.blobs_.begin(), tag.blobs_.end(), task->blob_id_);
    tag.blobs_.erase(blob_it);
    return task->SetComplete();
  }

  /** Clear blobs from a tag */
  void TagClearBlobs(MultiQueue *queue, TagClearBlobsTask *task) {
    auto it = tag_map_.find(task->tag_id_);
    if (it == tag_map_.end()) {
      task->SetComplete();
      return;
    }
    TagInfo &tag = it->second;
    tag.blobs_.clear();
    return task->SetComplete();
  }

  /**====================================
   * Blob Operations
   * ===================================*/

  /**
   * Create a blob's metadata
   * */
  void PutBlob(MultiQueue *queue, PutBlobTask *task) {
    // Get the blob info data structure
    task->did_create_ = false;
    if (task->blob_name_.size() > 0) {
      auto it = blob_id_map_.find(task->blob_name_);
      if (it == blob_id_map_.end()) {
        task->did_create_ = true;
      }
    } else {
      auto it = blob_map_.find(task->blob_id_);
      if (it == blob_map_.end()) {
        task->did_create_ = true;
      }
    }

    if (task->did_create_) {
      // Create new blob and emplace in map
      BlobId blob_id(node_id_, id_alloc_.fetch_add(1));
      blob_map_.emplace(blob_id, BlobInfo());
      task->blob_id_ = blob_id;
    }

    BlobInfo &blob_info = blob_map_[task->blob_id_];
    if (task->did_create_) {
      // Update blob info
      blob_info.name_ = task->blob_name_;
      blob_info.blob_id_ = task->blob_id_;
      blob_info.tag_id_ = task->tag_id_;
      blob_info.blob_size_ = task->data_size_;
      blob_info.score_ = task->score_;
      blob_info.mod_count_ = 0;
      blob_info.access_freq_ = 0;
      blob_info.last_flush_ = 0;
      blob_info.UpdateWriteStats();
    } else {
      // Modify existing blob
      blob_info.UpdateWriteStats();
    }

    // TODO(llogan)
    // Allocate new buffers using DPE if necessary
    BufferInfo &disk_buf = blob_info.buffers_.back();
    size_t max_cur_space = disk_buf.blob_off_ + disk_buf.t_size_;
    size_t needed_space = task->blob_off_ + task->data_size_;
    if (max_cur_space < needed_space) {
      // Allocate more buffers
      size_t size_diff = needed_space - max_cur_space;
      disk_buf.blob_size_ = needed_space;
    }
    // Modify blob data
  }

  /** Get a blob's data */
  void GetBlob(MultiQueue *queue, GetBlobTask *task) {
    // TODO(llogan)
  }

  /**
   * Tag a blob
   * */
  void TagBlob(MultiQueue *queue, TagBlobTask *task) {
    auto it = blob_map_.find(task->blob_id_);
    if (it == blob_map_.end()) {
      task->SetComplete();
      return;
    }
    BlobInfo &blob = it->second;
    blob.tags_.push_back(task->tag_);
    return task->SetComplete();
  }

  /**
   * Check if blob has a tag
   * */
  void BlobHasTag(MultiQueue *queue, BlobHasTagTask *task) {
    auto it = blob_map_.find(task->blob_id_);
    if (it == blob_map_.end()) {
      task->SetComplete();
      return;
    }
    BlobInfo &blob = it->second;
    task->has_tag_ = std::find(blob.tags_.begin(),
                               blob.tags_.end(),
                               task->tag_) != blob.tags_.end();
    return task->SetComplete();
  }

  /**
   * Get \a blob_name BLOB from \a bkt_id bucket
   * */
  void GetBlobId(MultiQueue *queue, GetBlobIdTask *task) {
    auto it = blob_id_map_.find(hshm::to_charbuf(*task->blob_name_));
    if (it == blob_id_map_.end()) {
      task->SetComplete();
      return;
    }
    task->blob_id_ = it->second;
    return task->SetComplete();
  }

  /**
   * Get \a blob_name BLOB name from \a blob_id BLOB id
   * */
  void GetBlobName(MultiQueue *queue, GetBlobNameTask *task) {
    auto it = blob_map_.find(task->blob_id_);
    if (it == blob_map_.end()) {
      task->SetComplete();
      return;
    }
    BlobInfo &blob = it->second;
    (*task->blob_name_) = blob.name_;
    return task->SetComplete();
  }

  /**
   * Get \a score from \a blob_id BLOB id
   * */
  void GetBlobScore(MultiQueue *queue, GetBlobScoreTask *task) {
    auto it = blob_map_.find(task->blob_id_);
    if (it == blob_map_.end()) {
      task->SetComplete();
      return;
    }
    BlobInfo &blob = it->second;
    task->score_ = blob.score_;
    return task->SetComplete();
  }

  /**
   * Get \a blob_id blob's buffers
   * */
  void GetBlobBuffers(MultiQueue *queue, GetBlobBuffersTask *task) {
    auto it = blob_map_.find(task->blob_id_);
    if (it == blob_map_.end()) {
      task->SetComplete();
      return;
    }
    BlobInfo &blob = it->second;
    (*task->buffers_) = blob.buffers_;
    return task->SetComplete();
  }

  /**
   * Rename \a blob_id blob to \a new_blob_name new blob name
   * in \a bkt_id bucket.
   * */
  void RenameBlob(MultiQueue *queue, RenameBlobTask *task) {
    auto it = blob_map_.find(task->blob_id_);
    if (it == blob_map_.end()) {
      task->SetComplete();
      return;
    }
    BlobInfo &blob = it->second;
    blob_id_map_.erase(blob.name_);
    blob_id_map_[blob.name_] = task->blob_id_;
    blob.name_ = hshm::to_charbuf(*task->new_blob_name_);
    return task->SetComplete();
  }

  /**
   * Truncate a blob to a new size
   * */
  void TruncateBlob(MultiQueue *queue, TruncateBlobTask *task) {
    auto it = blob_map_.find(task->blob_id_);
    if (it == blob_map_.end()) {
      task->SetComplete();
      return;
    }
    BlobInfo &blob = it->second;
    // TODO(llogan): truncate blob
    return task->SetComplete();
  }

  /**
   * Destroy \a blob_id blob in \a bkt_id bucket
   * */
  void DestroyBlob(MultiQueue *queue, DestroyBlobTask *task) {
    auto it = blob_map_.find(task->blob_id_);
    if (it == blob_map_.end()) {
      task->SetComplete();
      return;
    }
    BlobInfo &blob = it->second;
    blob_id_map_.erase(blob.name_);
    blob_map_.erase(it);
    return task->SetComplete();
  }
};

}  // namespace labstor

LABSTOR_TASK_CC(hermes::mdm::Server, "hermes_mdm");
