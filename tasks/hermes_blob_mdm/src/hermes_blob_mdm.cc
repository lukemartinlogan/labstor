//
// Created by lukemartinlogan on 6/29/23.
//
#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "hermes/config_server.h"
#include "hermes_blob_mdm/hermes_blob_mdm.h"
#include "hermes/dpe/dpe_factory.h"
#include "bdev/bdev.h"

namespace hermes::blob_mdm {

/** Type name simplification for the various map types */
typedef std::unordered_map<hshm::charbuf, BlobId> BLOB_ID_MAP_T;
typedef std::unordered_map<BlobId, BlobInfo> BLOB_MAP_T;
typedef hipc::mpsc_queue<IoStat> IO_PATTERN_LOG_T;

class Server : public TaskLib {
 public:
  /**====================================
   * Configuration
   * ===================================*/
  u32 node_id_;

  /**====================================
   * Maps
   * ===================================*/
  BLOB_ID_MAP_T blob_id_map_;
  BLOB_MAP_T blob_map_;
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
  std::vector<CreateTaskStateTask*> target_tasks_;
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
    id_alloc_ = 0;
    node_id_ = LABSTOR_QM_CLIENT->node_id_;
    switch (task->phase_) {
      case ConstructTaskPhase::kCreateTaskStates: {
        target_tasks_.reserve(HERMES->server_config_.devices_.size());
        for (DeviceInfo &dev : HERMES->server_config_.devices_) {
          std::string dev_type;
          if (dev.mount_dir_.empty()) {
            dev_type = "ram_bdev";
            dev.mount_point_ = hshm::Formatter::format("{}/{}", dev.mount_dir_, dev.dev_name_);
          } else {
            dev_type = "posix_bdev";
          }
          targets_.emplace_back();
          bdev::Client &client = targets_.back();
          auto *create_task = client.AsyncCreate(
              task->task_node_,
              DomainId::GetLocal(),
              "hermes_" + dev.dev_name_,
              dev_type,
              dev);
          target_tasks_.emplace_back(create_task);
        }
        task->phase_ = ConstructTaskPhase::kWaitForTaskStates;
      }

      case ConstructTaskPhase::kWaitForTaskStates: {
        HILOG(kInfo, "Wait for states")
        for (auto &tgt_task : target_tasks_) {
          if (!tgt_task->IsComplete()) {
            return;
          }
        }
        for (int i = 0; i < target_tasks_.size(); ++i) {
          auto &tgt_task = target_tasks_[i];
          auto &client = targets_[i];
          client.id_ = tgt_task->id_;
          HILOG(kInfo, "Client ID: {}", client.id_)
          LABSTOR_CLIENT->DelTask(tgt_task);
          target_map_.emplace(client.id_, &client);
        }
      }
    }

    // Create targets
    HILOG(kInfo, "Created MDM")
    task->SetComplete();
  }

  void Destruct(MultiQueue *queue, DestructTask *task) {
    task->SetComplete();
  }

 private:
  /** Get the globally unique blob name */
  const hshm::charbuf GetBlobNameWithBucket(TagId tag_id, const hshm::charbuf &blob_name) {
    hshm::charbuf new_name(blob_name.size());
    size_t off = 0;
    memcpy(new_name.data() + off, &tag_id, sizeof(TagId));
    off += sizeof(TagId);
    memcpy(new_name.data() + off, blob_name.data(), blob_name.size());
    return new_name;
  }

 public:
  /**
   * Create a blob's metadata
   * */
  void PutBlob(MultiQueue *queue, PutBlobTask *task) {
    switch (task->phase_) {
      case PutBlobPhase::kCreate: {
        HILOG(kDebug, "PutBlobPhase::kCreate");
        // Get the blob info data structure
        task->did_create_ = false;
        hshm::charbuf blob_name = hshm::to_charbuf(*task->blob_name_);
        hshm::charbuf blob_name_unique;
        if (blob_name.size() > 0) {
          blob_name_unique = GetBlobNameWithBucket(task->tag_id_, blob_name);
          auto it = blob_id_map_.find(blob_name);
          if (it == blob_id_map_.end()) {
            task->did_create_ = true;
          } else {
            task->blob_id_ = it->second;
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
          blob_id_map_.emplace(blob_name_unique, blob_id);
          blob_map_.emplace(blob_id, BlobInfo());
          task->blob_id_ = blob_id;
        }

        // Complete enough for the user
        task->SetUserComplete();

        BlobInfo &blob_info = blob_map_[task->blob_id_];
        if (task->did_create_) {
          // Update blob info
          blob_info.name_ = std::move(blob_name);
          blob_info.blob_id_ = task->blob_id_;
          blob_info.tag_id_ = task->tag_id_;
          blob_info.blob_size_ = task->data_size_;
          blob_info.max_blob_size_ = 0;
          blob_info.score_ = task->score_;
          blob_info.mod_count_ = 0;
          blob_info.access_freq_ = 0;
          blob_info.last_flush_ = 0;
          blob_info.UpdateWriteStats();
        } else {
          // Modify existing blob
          blob_info.UpdateWriteStats();
        }
        if (task->flags_.Any(HERMES_BLOB_REPLACE)) {
          // TODO(llogan): destroy blob buffers
        }

        // Determine amount of additional buffering space needed
        Context ctx;
        size_t size_diff;
        HSHM_MAKE_AR0(task->schema_, nullptr);
        std::vector<PlacementSchema> *schema = task->schema_.get();
        if (blob_info.max_blob_size_ > 0) {
          size_t needed_space = task->blob_off_ + task->data_size_;
          if (blob_info.max_blob_size_ < needed_space) {
            size_diff = needed_space - blob_info.max_blob_size_;
          }
        } else {
          size_diff = task->blob_off_ + task->data_size_;
        }

        // Use DPE
        if (size_diff > 0) {
          auto *dpe = DpeFactory::Get(ctx.dpe_);
          dpe->Placement({size_diff}, targets_, ctx, *schema);
        } else {
          task->phase_ = PutBlobPhase::kModify;
          return;
        }
      }

      case PutBlobPhase::kAllocate: {
        BlobInfo &blob_info = blob_map_[task->blob_id_];
        PlacementSchema &schema = (*task->schema_)[task->plcmnt_idx_];
        SubPlacement &placement = schema.plcmnts_[task->sub_plcmnt_idx_];
        TargetInfo &bdev = *target_map_[placement.tid_];
        HILOG(kDebug, "Allocating {} bytes of blob {}", placement.size_, task->blob_id_);
        task->cur_bdev_alloc_ = bdev.AsyncAllocate(task->task_node_,
                                                   placement.size_,
                                                   blob_info.buffers_);
        task->phase_ = PutBlobPhase::kWaitAllocate;
      }

      case PutBlobPhase::kWaitAllocate: {
        if (!task->cur_bdev_alloc_->IsComplete()){
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
        if (task->cur_bdev_alloc_->alloc_size_ < task->cur_bdev_alloc_->size_) {
          size_t diff = task->cur_bdev_alloc_->size_ - task->cur_bdev_alloc_->alloc_size_;
          // TODO(llogan): Pass remaining capacity to next schema
        }
        if (task->plcmnt_idx_ < task->schema_->size()) {
          task->phase_ = PutBlobPhase::kAllocate;
          return;
        } else {
          task->phase_ = PutBlobPhase::kModify;
          HSHM_DESTROY_AR(task->schema_);
          HSHM_MAKE_AR0(task->bdev_writes_, nullptr);
          task->bdev_writes_->reserve(blob_info.buffers_.size());
        }
      }

      case PutBlobPhase::kModify: {
        BlobInfo &blob_info = blob_map_[task->blob_id_];
        hipc::mptr<char> blob_data_mptr(task->data_);
        char *blob_data = blob_data_mptr.get();
        std::vector<bdev::WriteTask*> &write_tasks = *task->bdev_writes_;
        size_t blob_off = 0;
        for (BufferInfo &buf : blob_info.buffers_) {
          if (blob_off <= task->blob_off_ &&
              task->blob_off_ + task->data_size_ <= blob_off + buf.t_size_) {
            size_t rel_off = task->blob_off_ - blob_off;
            size_t tgt_off = buf.t_off_ + rel_off;
            size_t buf_size = buf.t_size_ - rel_off;
            TargetInfo &target = *target_map_[buf.tid_];
            auto write_task = target.AsyncWrite(task->task_node_,
                                                blob_data + blob_off, tgt_off, buf_size);
            write_tasks.emplace_back(write_task);
          }
          blob_off += buf.t_size_;
        }
        blob_info.max_blob_size_ = blob_off;
        task->phase_ = PutBlobPhase::kWaitModify;
        HILOG(kDebug, "Modified {} bytes of blob {}", blob_off, task->blob_id_);
      }

      case PutBlobPhase::kWaitModify: {
        std::vector<bdev::WriteTask*> &write_tasks = *task->bdev_writes_;
        for (auto &write_task : write_tasks) {
          if (!write_task->IsComplete()) {
            return;
          }
        }
        for (auto &write_task : write_tasks) {
          LABSTOR_CLIENT->DelTask(write_task);
        }
        HSHM_DESTROY_AR(task->bdev_writes_);
        HILOG(kDebug, "PutBlobTask complete");
        task->SetComplete();
      }
    }
  }

  /** Get a blob's data */
  void GetBlob(MultiQueue *queue, GetBlobTask *task) {
    switch (task->phase_) {
      case GetBlobPhase::kStart: {
        HILOG(kDebug, "GetBlobTask start");
        BlobInfo &blob_info = blob_map_[task->blob_id_];
        HSHM_MAKE_AR0(task->bdev_reads_, nullptr);
        std::vector<bdev::ReadTask*> &read_tasks = *task->bdev_reads_;
        read_tasks.reserve(blob_info.buffers_.size());
        size_t blob_off = 0;
        if (task->data_size_ < 0) {
          task->data_size_ = (ssize_t)(blob_info.blob_size_ - task->blob_off_);
        }
        task->data_ = LABSTOR_CLIENT->AllocateBuffer(task->data_size_);
        hipc::mptr<char> blob_data_mptr(task->data_);
        char *blob_data = blob_data_mptr.get();
        for (BufferInfo &buf : blob_info.buffers_) {
          if (blob_off <= task->blob_off_ &&
              task->blob_off_ + task->data_size_ <= blob_off + buf.t_size_) {
            size_t rel_off = task->blob_off_ - blob_off;
            size_t tgt_off = buf.t_off_ + rel_off;
            size_t buf_size = buf.t_size_ - rel_off;
            TargetInfo &target = *target_map_[buf.tid_];
            auto read_task = target.AsyncRead(task->task_node_,
                                              blob_data + blob_off, tgt_off, buf_size);
            read_tasks.emplace_back(read_task);
          }
          blob_off += buf.t_size_;
        }
        task->phase_ = GetBlobPhase::kWait;
      }
      case GetBlobPhase::kWait: {
        std::vector<bdev::ReadTask*> &read_tasks = *task->bdev_reads_;
        for (auto &read_task : read_tasks) {
          if (!read_task->IsComplete()) {
            return;
          }
        }
        for (auto &read_task : read_tasks) {
          LABSTOR_CLIENT->DelTask(read_task);
        }
        HSHM_DESTROY_AR(task->bdev_reads_);
        HILOG(kDebug, "GetBlobTask complete");
        task->SetComplete();
      }
    }
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
    hshm::charbuf blob_name = hshm::to_charbuf(*task->blob_name_);
    hshm::charbuf blob_name_unique = GetBlobNameWithBucket(task->tag_id_, blob_name);
    auto it = blob_id_map_.find(blob_name_unique);
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
    BlobInfo &blob_info = it->second;
    // TODO(llogan): truncate blob
    return task->SetComplete();
  }

  /**
   * Destroy \a blob_id blob in \a bkt_id bucket
   * */
  void DestroyBlob(MultiQueue *queue, DestroyBlobTask *task) {
    switch (task->phase_) {
      case DestroyBlobPhase::kFreeBuffers: {
        auto it = blob_map_.find(task->blob_id_);
        if (it == blob_map_.end()) {
          task->SetComplete();
          return;
        }
        BlobInfo &blob_info = it->second;
        blob_id_map_.erase(blob_info.name_);
        task->blob_info_ = &blob_info;
        HSHM_MAKE_AR0(task->free_tasks_, LABSTOR_CLIENT->main_alloc_);
        task->free_tasks_->reserve(blob_info.buffers_.size());
        for (BufferInfo &buf : blob_info.buffers_) {
          TargetInfo &tgt_info = *target_map_[buf.tid_];
          auto *free_task = tgt_info.AsyncFree(task->task_node_, {buf});
          task->free_tasks_->emplace_back(free_task);
        }
      }
      case DestroyBlobPhase::kWaitFreeBuffers: {
        for (auto *free_task : *task->free_tasks_) {
          if (!free_task->IsComplete()) {
            return;
          }
        }
        HSHM_DESTROY_AR(task->free_tasks_);
        blob_map_.erase(task->blob_id_);
      }
    }
    return task->SetComplete();
  }
};

}  // namespace labstor

LABSTOR_TASK_CC(hermes::blob_mdm::Server, "hermes_blob_mdm");
