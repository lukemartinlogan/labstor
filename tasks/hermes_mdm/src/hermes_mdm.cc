//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "hermes/config_server.h"
#include "hermes_mdm/hermes_mdm.h"
#include "bdev/bdev.h"

namespace hermes::mdm {

/** Type name simplification for the various map types */
typedef std::unordered_map<std::string, BlobId> BLOB_ID_MAP_T;
typedef std::unordered_map<std::string, TagId> TAG_ID_MAP_T;
typedef std::unordered_map<BlobId, BlobInfo> BLOB_MAP_T;
typedef std::unordered_map<TagId, TagInfo> TAG_MAP_T;
typedef hipc::mpsc_queue<IoStat> IO_PATTERN_LOG_T;

class Server : public TaskLib {
 public:
  /**====================================
   * Configuration
   * ===================================*/
   ServerConfig server_config_;

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

  /** Get or create a tag */
  void GetOrCreateTag(MultiQueue *queue, GetOrCreateTagTask *task) {
    TagId tag_id;

    // Check if the tag exists
    std::string tag_name = task->tag_name_->str();
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
  }
};

}  // namespace labstor

LABSTOR_TASK_CC(hermes::mdm::Server, "hermes_mdm");
