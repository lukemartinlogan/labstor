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
  create_state_t create_state_;   /**< The create task function */
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
                       create_state_t create_task,
                       get_task_lib_name_t get_task_name)
      : lib_(lib), create_state_(create_task), get_task_lib_name(get_task_name) {}

  /** Copy constructor */
  TaskLibInfo(const TaskLibInfo &other)
      : lib_(other.lib_),
        create_state_(other.create_state_),
        get_task_lib_name(other.get_task_lib_name) {}

  /** Move constructor */
  TaskLibInfo(TaskLibInfo &&other) noexcept
      : lib_(other.lib_),
        create_state_(other.create_state_),
        get_task_lib_name(other.get_task_lib_name) {
    other.lib_ = nullptr;
    other.create_state_ = nullptr;
    other.get_task_lib_name = nullptr;
  }
};

/**
 * Stores the registered set of TaskLibs and TaskStates
 * */
class TaskRegistry {
 public:
  /** The dirs to search for task libs */
  std::vector<std::string> lib_dirs_;
  /** Map of a semantic lib name to lib info */
  std::unordered_map<std::string, TaskLibInfo> libs_;
  /** Map of a semantic exec name to exec id */
  std::unordered_map<std::string, TaskStateId> task_state_ids_;
  /** Map of a semantic exec id to state */
  std::unordered_map<TaskStateId, TaskState*> task_states_;
  /** A unique identifier counter */
  std::atomic<u64> unique_;

 public:
  /** Default constructor */
  TaskRegistry() : unique_(0) {}

  /** Initialize the Task Registry */
  void ServerInit(ServerConfig *config) {
    // Load the LD_LIBRARY_PATH variable
    auto ld_lib_path_env = getenv("LD_LIBRARY_PATH");
    std::string ld_lib_path;
    if (ld_lib_path_env) {
      ld_lib_path = ld_lib_path_env;
    }

    // Load the LABSTOR_TASK_PATH variable
    std::string hermes_lib_path;
    auto hermes_lib_path_env = getenv("LABSTOR_TASK_PATH");
    if (hermes_lib_path_env) {
      hermes_lib_path = hermes_lib_path_env;
    }

    // Combine LD_LIBRARY_PATH and LABSTOR_TASK_PATH
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
        HELOG(kError, "Could not open the lib library: {}. Reason: {}", lib_path, dlerror());
        return false;
      }
      info.create_state_ = (create_state_t)dlsym(
          info.lib_, "create_state");
      if (!info.create_state_) {
        HELOG(kError, "The lib {} does not have create_state symbol",
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
    HELOG(kError, "Could not find the lib: {}", lib_name);
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

  /** Get a TaskState ID */
  TaskStateId CreateTaskStateId(u32 node_id) {
    return TaskStateId(unique_.fetch_add(1), node_id);;
  }

  /** Create a task state */
  TaskStateId CreateTaskState(const char *lib_name,
                             const char *state_name,
                             u32 node_id,
                             const TaskStateId &state_id,
                             Task *task) {
    // Find the task library to instantiate
    auto it = libs_.find(lib_name);
    if (it == libs_.end()) {
      HELOG(kError, "Could not find the task lib: {}", lib_name);
      return TaskStateId::GetNull();
    }

    // Check that the state doesn't already exist
    if (state_name && task_state_ids_.find(state_name) != task_state_ids_.end()) {
      HELOG(kError, "The task state already exists: {}", state_name);
      return TaskStateId::GetNull();
    }

    // Create the state instance
    TaskLibInfo &info = it->second;
    TaskState *task_state = info.create_state_(task);
    if (!task_state) {
      HELOG(kError, "Could not create the task state: {}", state_name);
      return TaskStateId::GetNull();
    }

    // Add the state to the registry
    task_state->id_ = state_id;
    task_state_ids_.emplace(state_name, state_id);
    task_states_.emplace(state_id, task_state);
    return state_id;
  }

  /** Get a task state's ID */
  TaskStateId GetTaskStateId(const std::string &state_name) {
    auto it = task_state_ids_.find(state_name);
    if (it == task_state_ids_.end()) {
      return TaskStateId::GetNull();
    }
    return it->second;
  }

  /** Get a task state instance */
  TaskState *GetTaskState(const TaskStateId &task_state_id) {
    auto it = task_states_.find(task_state_id);
    if (it == task_states_.end()) {
      return nullptr;
    }
    return it->second;
  }

  /** Destroy a task state */
  void DestroyTaskState(const TaskStateId &task_state_id) {
    auto it = task_states_.find(task_state_id);
    if (it == task_states_.end()) {
      HELOG(kWarning, "Could not find the task state");
      return;
    }
    TaskState *task_state = it->second;
    task_state_ids_.erase(task_state->name_);
    task_states_.erase(it);
    delete task_state;
  }
};

/** Singleton macro for task registry */
#define LABSTOR_TASK_REGISTRY \
  (&LABSTOR_RUNTIME->task_registry_)
}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_TASK_TASK_REGISTRY_H_
