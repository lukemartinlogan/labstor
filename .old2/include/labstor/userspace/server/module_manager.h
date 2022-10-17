//
// Created by lukemartinlogan on 11/4/21.
//

#ifndef LABSTOR_SERVER_MODULE_MANAGER_H
#define LABSTOR_SERVER_MODULE_MANAGER_H

#include <labstor/types/module.h>
#include <labstor/server/macros.h>
#include <labstor/server/server.h>
#include <set>

namespace labstor::Server {

class ModuleManager : public ModuleTable {
private:
    LABSTOR_CONFIGURATION_MANAGER_T labstor_config_;
    std::unordered_map<labstor::id, labstor::ModulePath> paths_;
    std::set<std::string> repos_;
public:
    ModuleManager() {
        labstor_config_ = LABSTOR_CONFIGURATION_MANAGER;
    }
    bool LoadRepos();
    void CentralizedUpdateModule(YAML::Node config);
    void DecentralizedUpdateModule(YAML::Node config);
    void AddModulePaths(labstor::id module_id, labstor::ModulePath paths);
    std::string GetModulePath(labstor::id module_id, ModulePathType type);
};

}

#endif //LABSTOR_MODULE_MANAGER_H
