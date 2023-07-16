//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "hermes/config_server.h"
#include "hermes_mdm/hermes_mdm.h"
#include "hermes/dpe/dpe_factory.h"
#include "bdev/bdev.h"

namespace hermes::mdm {

using labstor::Admin::CreateTaskStateInfo;

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
  std::vector<CreateTaskStateInfo> target_tasks_;
  std::vector<bdev::Client> targets_;
  std::unordered_map<TargetId, TargetInfo*> target_map_;

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
      case Method::kGetTagId: {
        GetTagId(queue, reinterpret_cast<GetTagIdTask *>(task));
        break;
      }
      case Method::kGetTagName: {
        GetTagName(queue, reinterpret_cast<GetTagNameTask *>(task));
        break;
      }
      case Method::kRenameTag: {
        RenameTag(queue, reinterpret_cast<RenameTagTask *>(task));
        break;
      }
      case Method::kDestroyTag: {
        DestroyTag(queue, reinterpret_cast<DestroyTagTask *>(task));
        break;
      }
      case Method::kTagAddBlob: {
        TagAddBlob(queue, reinterpret_cast<TagAddBlobTask *>(task));
        break;
      }
      case Method::kTagRemoveBlob: {
        TagRemoveBlob(queue, reinterpret_cast<TagRemoveBlobTask *>(task));
        break;
      }
      case Method::kTagClearBlobs: {
        TagClearBlobs(queue, reinterpret_cast<TagClearBlobsTask *>(task));
        break;
      }
      case Method::kPutBlob: {
        PutBlob(queue, reinterpret_cast<PutBlobTask *>(task));
        break;
      }
      case Method::kGetBlob: {
        GetBlob(queue, reinterpret_cast<GetBlobTask *>(task));
        break;
      }
      case Method::kTagBlob: {
        TagBlob(queue, reinterpret_cast<TagBlobTask *>(task));
        break;
      }
      case Method::kBlobHasTag: {
        BlobHasTag(queue, reinterpret_cast<BlobHasTagTask *>(task));
        break;
      }
      case Method::kGetBlobId: {
        GetBlobId(queue, reinterpret_cast<GetBlobIdTask *>(task));
        break;
      }
      case Method::kGetBlobName: {
        GetBlobName(queue, reinterpret_cast<GetBlobNameTask *>(task));
        break;
      }
      case Method::kGetBlobScore: {
        GetBlobScore(queue, reinterpret_cast<GetBlobScoreTask *>(task));
        break;
      }
      case Method::kGetBlobBuffers: {
        GetBlobBuffers(queue, reinterpret_cast<GetBlobBuffersTask *>(task));
        break;
      }
      case Method::kRenameBlob: {
        RenameBlob(queue, reinterpret_cast<RenameBlobTask *>(task));
        break;
      }
      case Method::kTruncateBlob: {
        TruncateBlob(queue, reinterpret_cast<TruncateBlobTask *>(task));
        break;
      }
      case Method::kDestroyBlob: {
        DestroyBlob(queue, reinterpret_cast<DestroyBlobTask *>(task));
        break;
      }
    }
  }

  void Construct(MultiQueue *queue, ConstructTask *task) {
    switch (task->phase_) {
      case ConstructTaskPhase::kLoadConfig: {
        id_alloc_ = 0;
        std::string config_path = task->server_config_path_->str();

        // Load hermes config
        if (config_path.empty()) {
          config_path = GetEnvSafe(Constant::kHermesServerConf);
        }
        HILOG(kInfo, "Loading server configuration: {}", config_path)
        server_config_.LoadFromFile(config_path);
        node_id_ = LABSTOR_QM_CLIENT->node_id_;
        task->phase_ = ConstructTaskPhase::kCreateTaskStates;
      }

      case ConstructTaskPhase::kCreateTaskStates: {
        target_tasks_.reserve(server_config_.devices_.size());
        for (DeviceInfo &dev : server_config_.devices_) {
          bdev::Client client;
          std::string dev_type;
          if (dev.mount_dir_.empty()) {
            dev_type = "ram_bdev";
            dev.mount_point_ = hshm::Formatter::format("{}/{}", dev.mount_dir_, dev.dev_name_);
          } else {
            dev_type = "posix_bdev";
          }
          target_tasks_.emplace_back();
          client.ACreateTaskState(DomainId::GetLocal(),
                                  "hermes_" + dev.dev_name_,
                                  dev_type,
                                  dev,
                                  target_tasks_.back());
          targets_.emplace_back(std::move(client));
          target_map_.emplace(targets_.back().id_, &targets_.back());
        }
        task->phase_ = ConstructTaskPhase::kWaitForTaskStates;
      }

      case ConstructTaskPhase::kWaitForTaskStates: {
        HILOG(kDebug, "Wait for states")
        for (auto &tgt_task : target_tasks_) {
          if (!tgt_task.state_task_->IsComplete()) {
            return;
          }
        }
        task->phase_ = ConstructTaskPhase::kCreateQueues;
      }

      case ConstructTaskPhase::kCreateQueues: {
        HILOG(kDebug, "Create queues")
        int i = 0;
        for (auto &client : targets_) {
          auto &tgt_task = target_tasks_[i];
          client.ACreateQueue(DomainId::GetLocal(), tgt_task);
          LABSTOR_CLIENT->DelTask(tgt_task.state_task_);
          ++i;
        }
        task->phase_ = ConstructTaskPhase::kWaitForQueues;
      }

      case ConstructTaskPhase::kWaitForQueues: {
        HILOG(kDebug, "Wait for queues")
        for (auto &tgt_task : target_tasks_) {
          if (!tgt_task.queue_task_->IsComplete()) {
            return;
          }
          LABSTOR_CLIENT->DelTask(tgt_task.queue_task_);
        }
      }
    }

    // Create targets
    HILOG(kDebug, "Created MDM")
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
    bool did_create = false;
    if (tag_name.size() > 0) {
      did_create = tag_id_map_.find(tag_name) == tag_id_map_.end();
    }

    // Emplace bucket if it does not already exist
    if (did_create) {
      tag_id.unique_ = id_alloc_.fetch_add(1);
      tag_id.node_id_ = LABSTOR_RUNTIME->rpc_.node_id_;
      HILOG(kDebug, "Creating tag for the first time: {} {}", tag_name.str(), tag_id)
      tag_id_map_.emplace(tag_name, tag_id);
      tag_map_.emplace(tag_id, TagInfo());
      TagInfo &info = tag_map_[tag_id];
      info.name_ = tag_name;
      info.tag_id_ = tag_id;
      info.owner_ = task->blob_owner_;
      info.internal_size_ = task->backend_size_;
    } else {
      if (tag_name.size()) {
        HILOG(kDebug, "Found existing tag: {}", tag_name.str())
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
    switch (task->phase_) {
      case PutBlobPhase::kCreate: {
        HILOG(kDebug, "PutBlobPhase::kCreate");
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
        // Use DPE to decide how much space from each target to allocate
        BufferInfo &disk_buf = blob_info.buffers_.back();
        size_t max_cur_space = disk_buf.blob_off_ + disk_buf.t_size_;
        size_t needed_space = task->blob_off_ + task->data_size_;
        HSHM_MAKE_AR0(task->schema_, nullptr);
        std::vector<PlacementSchema> *output = task->schema_.get();
        if (max_cur_space < needed_space) {
          Context ctx;
          size_t size_diff = needed_space - max_cur_space;
          disk_buf.blob_size_ = needed_space;
          auto *dpe =  DpeFactory::Get(ctx.dpe_);
          dpe->Placement({size_diff}, targets_, ctx, *output);
        }
      }

      case PutBlobPhase::kAllocate: {
        HILOG(kDebug, "PutBlobPhase::kAllocate");
        BlobInfo &blob_info = blob_map_[task->blob_id_];
        PlacementSchema &schema = (*task->schema_)[task->plcmnt_idx_];
        SubPlacement &placement = schema.plcmnts_[task->sub_plcmnt_idx_];
        TargetInfo &bdev = *target_map_[placement.tid_];
        task->cur_bdev_alloc_ = bdev.AsyncAllocate(placement.size_,
                                                   blob_info.buffers_);
        task->phase_ = PutBlobPhase::kWaitAllocate;
      }

      case PutBlobPhase::kWaitAllocate: {
        HILOG(kDebug, "PutBlobPhase::kWaitAllocate");
        if (!task->cur_bdev_alloc_->IsComplete()){
          return;
        }
        if (task->cur_bdev_alloc_->alloc_size_ < task->cur_bdev_alloc_->size_) {
          size_t diff = task->cur_bdev_alloc_->size_ - task->cur_bdev_alloc_->alloc_size_;
          // TODO(llogan): Pass remaining capacity to next schema
          return;
        }
        LABSTOR_CLIENT->DelTask(task->cur_bdev_alloc_);
        BlobInfo &blob_info = blob_map_[task->blob_id_];
        PlacementSchema &schema = (*task->schema_)[task->plcmnt_idx_];
        ++task->sub_plcmnt_idx_;
        if (task->sub_plcmnt_idx_ >= schema.plcmnts_.size()) {
          ++task->plcmnt_idx_;
          task->sub_plcmnt_idx_ = 0;
        }
        if (task->plcmnt_idx_ < task->schema_->size()) {
          task->phase_ = PutBlobPhase::kAllocate;
          return;
        } else {
          task->phase_ = PutBlobPhase::kModify;
          HSHM_MAKE_AR(task->bdev_writes_, nullptr, blob_info.buffers_.size());
        }
      }

      case PutBlobPhase::kModify: {
        HILOG(kDebug, "PutBlobPhase::kModify");
        BlobInfo &blob_info = blob_map_[task->blob_id_];
        hipc::mptr<char> blob_data_mptr(task->data_);
        char *blob_data = blob_data_mptr.get();
        std::vector<bdev::WriteTask*> &write_tasks = *task->bdev_writes_;
        for (BufferInfo &buf : blob_info.buffers_) {
          if (buf.blob_off_ <= task->blob_off_ &&
              task->blob_off_ + task->data_size_ <= buf.blob_off_ + buf.blob_size_) {
            size_t rel_off = task->blob_off_ - buf.blob_off_;
            size_t tgt_off = buf.t_off_ + rel_off;
            size_t buf_size = buf.t_size_ - rel_off;
            TargetInfo &target = *target_map_[buf.tid_];
            auto write_task = target.AsyncWrite(blob_data, tgt_off, buf_size);
            write_tasks.emplace_back(write_task);
          }
        }
        task->phase_ = PutBlobPhase::kWaitModify;
      }

      case PutBlobPhase::kWaitModify: {
        HILOG(kDebug, "PutBlobPhase::kWaitModify");
        std::vector<bdev::WriteTask*> &write_tasks = *task->bdev_writes_;
        for (auto &write_task : write_tasks) {
          if (!write_task->IsComplete()) {
            return;
          } else {
            LABSTOR_CLIENT->DelTask(write_task);
          }
        }
        task->SetComplete();
      }
    }
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
