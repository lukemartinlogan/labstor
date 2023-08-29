#ifndef LABSTOR_REMOTE_QUEUE_LIB_EXEC_H_
#define LABSTOR_REMOTE_QUEUE_LIB_EXEC_H_

/** Execute a task */
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
    case Method::kPush: {
      Push(queue, reinterpret_cast<PushTask *>(task));
      break;
    }
  }
}
/** Ensure there is space to store replicated outputs */
void ReplicateStart(u32 method, u32 count, Task *task) override {
  switch (method) {
    case Method::kConstruct: {
      if constexpr(USES_REPLICA(ConstructTask)) {
        reinterpret_cast<ConstructTask*>(task)->ReplicateStart(count);
      }
      break;
    }
    case Method::kDestruct: {
      if constexpr(USES_REPLICA(DestructTask)) {
        reinterpret_cast<DestructTask*>(task)->ReplicateStart(count);
      }
      break;
    }
    case Method::kPush: {
      if constexpr(USES_REPLICA(PushTask)) {
        reinterpret_cast<PushTask*>(task)->ReplicateStart(count);
      }
      break;
    }
  }
  return ar.Get();
}
/** Determine success and handle failures */
void ReplicateEnd(u32 method, Task *task) override {
  switch (method) {
    case Method::kConstruct: {
      if constexpr(USES_REPLICA(ConstructTask)) {
        reinterpret_cast<ConstructTask*>(task)->ReplicateEnd();
      }
      break;
    }
    case Method::kDestruct: {
      if constexpr(USES_REPLICA(DestructTask)) {
        reinterpret_cast<DestructTask*>(task)->ReplicateEnd();
      }
      break;
    }
    case Method::kPush: {
      if constexpr(USES_REPLICA(PushTask)) {
        reinterpret_cast<PushTask*>(task)->ReplicateEnd();
      }
      break;
    }
  }
  return ar.Get();
}
/** Serialize a task when initially pushing into remote */
std::vector<DataTransfer> SaveStart(u32 method, BinaryOutputArchive<true> &ar, Task *task) override {
  switch (method) {
    case Method::kConstruct: {
      ar << *reinterpret_cast<ConstructTask*>(task);
      break;
    }
    case Method::kDestruct: {
      ar << *reinterpret_cast<DestructTask*>(task);
      break;
    }
    case Method::kPush: {
      ar << *reinterpret_cast<PushTask*>(task);
      break;
    }
  }
  return ar.Get();
}
/** Deserialize a task when popping from remote queue */
TaskPointer LoadStart(u32 method, BinaryInputArchive<true> &ar) override {
  TaskPointer task_ptr;
  switch (method) {
    case Method::kConstruct: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<ConstructTask>(task_ptr.p_);
      ar >> *reinterpret_cast<ConstructTask*>(task_ptr.task_);
      break;
    }
    case Method::kDestruct: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<DestructTask>(task_ptr.p_);
      ar >> *reinterpret_cast<DestructTask*>(task_ptr.task_);
      break;
    }
    case Method::kPush: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<PushTask>(task_ptr.p_);
      ar >> *reinterpret_cast<PushTask*>(task_ptr.task_);
      break;
    }
  }
  return task_ptr;
}
/** Serialize a task when returning from remote queue */
std::vector<DataTransfer> SaveEnd(u32 method, BinaryOutputArchive<false> &ar, Task *task) override {
  switch (method) {
    case Method::kConstruct: {
      ar << *reinterpret_cast<ConstructTask*>(task);
      break;
    }
    case Method::kDestruct: {
      ar << *reinterpret_cast<DestructTask*>(task);
      break;
    }
    case Method::kPush: {
      ar << *reinterpret_cast<PushTask*>(task);
      break;
    }
  }
  return ar.Get();
}
/** Deserialize a task when returning from remote queue */
void LoadEnd(u32 replica, u32 method, BinaryInputArchive<false> &ar, Task *task) override {
  switch (method) {
    case Method::kConstruct: {
      ar.Deserialize(replica, *reinterpret_cast<ConstructTask*>(task));
      break;
    }
    case Method::kDestruct: {
      ar.Deserialize(replica, *reinterpret_cast<DestructTask*>(task));
      break;
    }
    case Method::kPush: {
      ar.Deserialize(replica, *reinterpret_cast<PushTask*>(task));
      break;
    }
  }
}

#endif  // LABSTOR_REMOTE_QUEUE_METHODS_H_