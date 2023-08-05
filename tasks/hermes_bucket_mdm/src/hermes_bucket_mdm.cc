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
    }
  }

  void Construct(MultiQueue *queue, ConstructTask *task) {
    id_ = task->id_;
    id_alloc_ = 0;
    node_id_ = LABSTOR_QM_CLIENT->node_id_;
    blob_mdm_.AsyncCreateRoot(DomainId::GetGlobal(), "hermes_blob_mdm");
    task->SetComplete();
  }

  void Destruct(MultiQueue *queue, DestructTask *task) {
    task->SetComplete();
  }

  /** Put a blob */
  void PutBlob(MultiQueue *queue, PutBlobTask *task) {
    switch (task->phase_) {
      case PutBlobPhase::kUpdateMdm: {
        TagInfo &tag_info = tag_map_[task->tag_id_];
        tag_info.internal_size_ = std::max(task->blob_off_ + task->data_size_,
                                           tag_info.internal_size_);
        hshm::charbuf blob_name = hshm::to_charbuf(*task->blob_name_);
        if (task->flags_.Any(HERMES_BLOB_APPEND)) {
          adapter::BlobPlacement p;
          p.page_ = tag_info.internal_size_ / tag_info.page_size_;
          blob_name = p.CreateBlobName();
        }
        task->blob_put_task_ = blob_mdm_.AsyncPutBlob(
            task->task_node_, task->tag_id_, blob_name,
            task->blob_id_, task->blob_off_, task->data_size_,
            task->data_, task->score_, task->flags_);
        task->phase_ = PutBlobPhase::kWait;
      }
      case PutBlobPhase::kWait: {
        if (task->blob_put_task_->IsComplete()) {
          task->SetComplete();
        }
        break;
      }
    }
  }

  /** Destroy a blob */
  // TODO(llogan)

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
};

}  // namespace labstor

LABSTOR_TASK_CC(hermes::bucket_mdm::Server, "hermes_bucket_mdm");
