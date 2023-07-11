//
// Created by lukemartinlogan on 7/9/23.
//

#ifndef LABSTOR_TASKS_HERMES_INCLUDE_HERMES_CONFIG_MANAGER_H_
#define LABSTOR_TASKS_HERMES_INCLUDE_HERMES_CONFIG_MANAGER_H_

#include "hermes_types.h"
#include "labstor_admin/labstor_admin.h"
#include "hermes_bpm/hermes_bpm.h"
#include "hermes_dpe/hermes_dpe.h"
#include "hermes_mdm/hermes_mdm.h"

namespace hermes {

class ConfigurationManager {
 public:
  mdm::Client mdm_;
  bpm::Client bpm_;
  dpe::Client dpe_;

 public:
  ConfigurationManager() = default;

  void ClientInit() {
    // Create connection to MDM
    mdm_.Create("hermes_mdm", DomainId::GetLocal());
    // Create connection to Dpe
    dpe_.Create("hermes_dpe", DomainId::GetLocal());
  }

  void ServerInit() {
    // Create connection to MDM
    mdm_.Create("hermes_mdm", DomainId::GetLocal());
    // Create connection to BPM
    bpm_.Create("hermes_bpm", DomainId::GetLocal());
    // Create connection to dpe
    dpe_.Create("hermes_dpe", DomainId::GetLocal());
  }
};

}  // namespace hermes

#define HERMES_CONF hshm::EasySingleton<ConfigurationManager>::GetInstance()

#endif  // LABSTOR_TASKS_HERMES_INCLUDE_HERMES_CONFIG_MANAGER_H_
