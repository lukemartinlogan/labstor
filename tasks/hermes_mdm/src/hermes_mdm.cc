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

class Server : public TaskLib {
 public:
  /**====================================
   * Configuration
   * ===================================*/
   u32 node_id_;

 public:
  Server() = default;

  void Construct(ConstructTask *task) {
    HILOG(kDebug, "ConstructTaskPhase::kLoadConfig")
    std::string config_path = task->server_config_path_->str();
    HERMES->LoadServerConfig(config_path);
    node_id_ = LABSTOR_CLIENT->node_id_;
    task->phase_ = ConstructTaskPhase::kCreateTaskStates;
    task->SetModuleComplete();
  }

  void Destruct(DestructTask *task) {
    task->SetModuleComplete();
  }

 public:
#include "hermes_mdm/hermes_mdm_lib_exec.h"
};

}  // namespace labstor

LABSTOR_TASK_CC(hermes::mdm::Server, "hermes_mdm");
