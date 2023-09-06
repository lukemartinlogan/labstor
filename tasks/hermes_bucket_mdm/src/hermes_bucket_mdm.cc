//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "hermes/config_server.h"
#include "hermes_bucket_mdm/hermes_bucket_mdm.h"
#include "hermes_adapters/mapper/abstract_mapper.h"
#include "hermes/dpe/dpe_factory.h"
#include "bdev/bdev.h"

namespace hermes::bucket_mdm {

typedef std::unordered_map<hshm::charbuf, TagId> TAG_ID_MAP_T;
typedef std::unordered_map<TagId, TagInfo> TAG_MAP_T;

class Server : public TaskLib {
 public:
  TAG_ID_MAP_T tag_id_map_;
  TAG_MAP_T tag_map_;
  u32 node_id_;
  std::atomic<u64> id_alloc_;
  blob_mdm::Client blob_mdm_;
  blob_mdm::ConstructTask *blob_mdm_task_;

 public:
  Server() = default;

  void Construct(MultiQueue *queue, ConstructTask *task) {
    switch (task->phase_) {
      case ConstructTaskPhase::kInit: {
        id_alloc_ = 0;
        node_id_ = LABSTOR_QM_CLIENT->node_id_;
        blob_mdm_task_ = blob_mdm_.AsyncCreateRoot(
            DomainId::GetGlobal(), "hermes_blob_mdm");
        task->phase_ = ConstructTaskPhase::kWait;
      }
      case ConstructTaskPhase::kWait: {
        if (blob_mdm_task_->IsComplete()) {
          HILOG(kDebug, "Bucket MDM created")
          blob_mdm_.AsyncCreateComplete(blob_mdm_task_);
          task->SetModuleComplete();
          return;
        }
      }
    }
  }

  void Destruct(MultiQueue *queue, DestructTask *task) {
    task->SetModuleComplete();
  }

  /** Put a blob */
  void PutBlob(MultiQueue *queue, PutBlobTask *task) {
    TagInfo &tag_info = tag_map_[task->tag_id_];
    tag_info.internal_size_ = std::max(task->blob_off_ + task->data_size_,
                                       tag_info.internal_size_);
    task->SetModuleComplete();
  }

  /** Destroy a blob */
  // TODO(llogan)

  /** Get or create a tag */
  void GetOrCreateTag(MultiQueue *queue, GetOrCreateTagTask *task) {
    TagId tag_id;
    HILOG(kDebug, "Creating a tag")

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
    task->SetModuleComplete();
  }

  /** Get tag ID */
  void GetTagId(MultiQueue *queue, GetTagIdTask *task) {
    hshm::charbuf tag_name = hshm::to_charbuf(*task->tag_name_);
    auto it = tag_id_map_.find(tag_name);
    if (it == tag_id_map_.end()) {
      task->tag_id_ = TagId::GetNull();
      task->SetModuleComplete();
      return;
    }
    task->tag_id_ = it->second;
    task->SetModuleComplete();
  }

  /** Get tag name */
  void GetTagName(MultiQueue *queue, GetTagNameTask *task) {
    auto it = tag_map_.find(task->tag_id_);
    if (it == tag_map_.end()) {
      task->SetModuleComplete();
      return;
    }
    (*task->tag_name_) = it->second.name_;
    task->SetModuleComplete();
  }

  /** Rename tag */
  void RenameTag(MultiQueue *queue, RenameTagTask *task) {
    auto it = tag_map_.find(task->tag_id_);
    if (it == tag_map_.end()) {
      task->SetModuleComplete();
      return;
    }
    (*task->tag_name_) = (*task->tag_name_);
    task->SetModuleComplete();
  }

  /** Destroy tag */
  void DestroyTag(MultiQueue *queue, DestroyTagTask *task) {
    switch (task->phase_) {
      case DestroyTagPhase::kDestroyBlobs: {
        TagInfo &tag = tag_map_[task->tag_id_];
        tag_id_map_.erase(tag.name_);
        HSHM_MAKE_AR0(task->destroy_blob_tasks_, nullptr);
        std::vector<blob_mdm::DestroyBlobTask*> blob_tasks = *task->destroy_blob_tasks_;
        blob_tasks.reserve(tag.blobs_.size());
        for (BlobId &blob_id : tag.blobs_) {
          blob_mdm::DestroyBlobTask *blob_task =
              blob_mdm_.AsyncDestroyBlob(task->task_node_ + 1, blob_id);
          blob_tasks.emplace_back(blob_task);
        }
        task->phase_ = DestroyTagPhase::kWaitDestroyBlobs;
        return;
      }
      case DestroyTagPhase::kWaitDestroyBlobs: {
        std::vector<blob_mdm::DestroyBlobTask*> blob_tasks = *task->destroy_blob_tasks_;
        for (auto it = blob_tasks.rbegin(); it != blob_tasks.rend(); ++it) {
          blob_mdm::DestroyBlobTask *blob_task = *it;
          if (!blob_task->IsComplete()) {
            return;
          }
          LABSTOR_CLIENT->DelTask(blob_task);
          blob_tasks.pop_back();
        }
        HSHM_DESTROY_AR(task->destroy_blob_tasks_);
        tag_map_.erase(task->tag_id_);
        task->SetModuleComplete();
      }
    }
  }

  /** Add a blob to a tag */
  void TagAddBlob(MultiQueue *queue, TagAddBlobTask *task) {
    auto it = tag_map_.find(task->tag_id_);
    if (it == tag_map_.end()) {
      task->SetModuleComplete();
      return;
    }
    TagInfo &tag = it->second;
    return task->SetModuleComplete();
  }

  /** Remove a blob from a tag */
  void TagRemoveBlob(MultiQueue *queue, TagRemoveBlobTask *task) {
    auto it = tag_map_.find(task->tag_id_);
    if (it == tag_map_.end()) {
      task->SetModuleComplete();
      return;
    }
    TagInfo &tag = it->second;
    auto blob_it = std::find(tag.blobs_.begin(), tag.blobs_.end(), task->blob_id_);
    tag.blobs_.erase(blob_it);
    return task->SetModuleComplete();
  }

  /** Clear blobs from a tag */
  void TagClearBlobs(MultiQueue *queue, TagClearBlobsTask *task) {
    auto it = tag_map_.find(task->tag_id_);
    if (it == tag_map_.end()) {
      task->SetModuleComplete();
      return;
    }
    TagInfo &tag = it->second;
    tag.blobs_.clear();
    return task->SetModuleComplete();
  }

 public:
#include "hermes_bucket_mdm/hermes_bucket_mdm_lib_exec.h"
};

}  // namespace labstor

LABSTOR_TASK_CC(hermes::bucket_mdm::Server, "hermes_bucket_mdm");
