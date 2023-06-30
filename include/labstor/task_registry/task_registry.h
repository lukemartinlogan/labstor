//
// Created by lukemartinlogan on 6/23/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_TASK_TASK_REGISTRY_H_
#define LABSTOR_INCLUDE_LABSTOR_TASK_TASK_REGISTRY_H_

#include <string>
#include <cstdlib>
#include <sstream>
#include <unordered_map>
#include <filesystem>
#include "task_lib.h"
#include "labstor/config/config_server.h"

namespace stdfs = std::filesystem;

namespace labstor {

/** All information needed to create a trait */
struct TaskLibInfo {
  void *lib_;  /**< The dlfcn library */
  create_executor_t create_executor_;   /**< The create task function */
  get_task_lib_name_t get_task_lib_name; /**< The get task name function */

  /** Default constructor */
  TaskLibInfo() = default;

  /** Destructor */
  ~TaskLibInfo() {
    if (lib_) {
      dlclose(lib_);
    }
  }

  /** Emplace constructor */
  explicit TaskLibInfo(void *lib,
                        create_executor_t create_task,
                        get_task_lib_name_t get_task_name)
    : lib_(lib), create_executor_(create_task), get_task_lib_name(get_task_name) {}

  /** Copy constructor */
  TaskLibInfo(const TaskLibInfo &other)
    : lib_(other.lib_),
      create_executor_(other.create_executor_),
      get_task_lib_name(other.get_task_lib_name) {}

  /** Move constructor */
  TaskLibInfo(TaskLibInfo &&other) noexcept
    : lib_(other.lib_),
      create_executor_(other.create_executor_),
      get_task_lib_name(other.get_task_lib_name) {
    other.lib_ = nullptr;
    other.create_executor_ = nullptr;
    other.get_task_lib_name = nullptr;
  }
};

/**
 * Stores the registered set of TaskLibs and TaskExecutors
 * */
class TaskRegistry {
 public:
  /** The dirs to search for task libs */
  std::vector<std::string> lib_dirs_;
  /** Map of a semantic lib name to lib info */
  std::unordered_map<std::string, TaskLibInfo> libs_;
  /** Map of a semantic exec name to exec id */
  std::unordered_map<std::string, TaskExecId> task_exec_ids_;
  /** Map of a semantic exec id to executor */
  std::unordered_map<TaskExecId, TaskExecutor*> task_execs_;
  /** A unique identifier counter */
  std::atomic<u64> unique_;

 public:
  /** Default constructor */
  TaskRegistry() : unique_(1) {}

  /** Initialize the Task Registry */
  void ServerInit(ServerConfig *config) {
    // Load the LD_LIBRARY_PATH variable
    auto ld_lib_path_env = getenv("LD_LIBRARY_PATH");
    std::string ld_lib_path;
    if (ld_lib_path_env) {
      ld_lib_path = ld_lib_path_env;
    }

    // Load the HERMES_TRAIT_PATH variable
    std::string hermes_lib_path;
    auto hermes_lib_path_env = getenv("HERMES_TRAIT_PATH");
    if (hermes_lib_path_env) {
      hermes_lib_path = hermes_lib_path_env;
    }

    // Combine LD_LIBRARY_PATH and HERMES_TRAIT_PATH
    std::string paths = hermes_lib_path + ":" + ld_lib_path;
    std::stringstream ss(paths);
    std::string lib_dir;
    while (std::getline(ss, lib_dir, ':')) {
      lib_dirs_.emplace_back(lib_dir);
    }

    // Find each lib in LD_LIBRARY_PATH
    for (const std::string &lib_name : config->task_libs_) {
      if (!RegisterTaskLib(lib_name)) {
        HELOG(kWarning, "Failed to load the lib: {}", lib_name);
      }
    }
  }

  /** Load a task lib */
  bool RegisterTaskLib(const std::string &lib_name) {
    std::string lib_dir;
    for (const std::string &lib_dir : lib_dirs_) {
      // Determine if this directory contains the library
      std::string lib_path1 = hshm::Formatter::format("{}/{}.so",
                                                       lib_dir,
                                                       lib_name);
      std::string lib_path2 = hshm::Formatter::format("{}/lib{}.so",
                                                      lib_dir,
                                                      lib_name);
      std::string lib_path;
      if (stdfs::exists(lib_path1)) {
        lib_path = std::move(lib_path1);
      } else if (stdfs::exists(lib_path2)) {
        lib_path = std::move(lib_path2);
      } else {
        continue;
      }

      // Load the library
      TaskLibInfo info;
      info.lib_ = dlopen(lib_path.c_str(), RTLD_GLOBAL | RTLD_NOW);
      if (!info.lib_) {
        HELOG(kError, "Could not open the lib library: {}", lib_path);
        return false;
      }
      info.create_executor_ = (create_executor_t)dlsym(
        info.lib_, "create_executor");
      if (!info.create_executor_) {
        HELOG(kError, "The lib {} does not have create_executor symbol",
              lib_path);
        return false;
      }
      info.get_task_lib_name = (get_task_lib_name_t)dlsym(
        info.lib_, "get_task_lib_name");
      if (!info.get_task_lib_name) {
        HELOG(kError, "The lib {} does not have get_task_lib_name symbol",
              lib_path);
        return false;
      }
      std::string task_lib_name = info.get_task_lib_name();
      HILOG(kDebug, "Finished loading the lib: {}", task_lib_name)
      libs_.emplace(task_lib_name, std::move(info));
      return true;
    }
    return false;
  }

  /** Destroy a task lib */
  void DestroyTaskLib(const std::string &lib_name) {
    auto it = libs_.find(lib_name);
    if (it == libs_.end()) {
      HELOG(kError, "Could not find the task lib: {}", lib_name);
      return;
    }
    libs_.erase(it);
  }

  /** Create a task executor */
  TaskExecId CreateTaskExecutor(const char *lib_name,
                                const char *exec_name,
                                u32 node_id,
                                TaskExecId &exec_id,
                                Task *task) {
    // Find the task library to instantiate
    auto it = libs_.find(lib_name);
    if (it == libs_.end()) {
      HELOG(kError, "Could not find the task lib", lib_name);
      return TaskExecId::GetNull();
    }

    // Check that the executor doesn't already exist
    if (exec_name && task_exec_ids_.find(exec_name) != task_exec_ids_.end()) {
      HELOG(kError, "The task executor already exists: {}", exec_name);
      return TaskExecId::GetNull();
    }

    // Allocate a TaskExecId
    if (exec_id.IsNull()) {
      exec_id = TaskExecId(unique_.fetch_add(1), node_id);
    }

    // Create the executor instance
    TaskLibInfo &info = it->second;
    TaskExecutor *task_exec = info.create_executor_(task);
    if (!task_exec) {
      HELOG(kError, "Could not create the task executor: {}", exec_name);
      return TaskExecId::GetNull();
    }

    // Add the executor to the registry
    TaskExecId task_exec_id = task_exec->id_;
    task_exec_ids_.emplace(exec_name, task_exec_id);
    task_execs_.emplace(task_exec_id, task_exec);
    return task_exec_id;
  }

  /** Get a task executor's ID */
  TaskExecId GetTaskExecutorId(const std::string &exec_name) {
    auto it = task_exec_ids_.find(exec_name);
    if (it == task_exec_ids_.end()) {
      return TaskExecId::GetNull();
    }
    return it->second;
  }

  /** Get a task executor instance */
  TaskExecutor *GetTaskExecutor(TaskExecId task_exec_id) {
    auto it = task_execs_.find(task_exec_id);
    if (it == task_execs_.end()) {
      return nullptr;
    }
    return it->second;
  }
};

/** Singleton macro for task registry */
#define LABSTOR_TASK_REGISTRY \
  (&LABSTOR_RUNTIME->task_registry_)
}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_TASK_TASK_REGISTRY_H_
