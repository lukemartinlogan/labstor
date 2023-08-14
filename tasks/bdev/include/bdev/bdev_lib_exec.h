#ifndef LABSTOR_BDEV_LIB_EXEC_H_
#define LABSTOR_BDEV_LIB_EXEC_H_

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
    case Method::kWrite: {
      Write(queue, reinterpret_cast<WriteTask *>(task));
      break;
    }
    case Method::kRead: {
      Read(queue, reinterpret_cast<ReadTask *>(task));
      break;
    }
    case Method::kAlloc: {
      Alloc(queue, reinterpret_cast<AllocTask *>(task));
      break;
    }
    case Method::kFree: {
      Free(queue, reinterpret_cast<FreeTask *>(task));
      break;
    }
    case Method::kMonitor: {
      Monitor(queue, reinterpret_cast<MonitorTask *>(task));
      break;
    }
    case Method::kUpdateCapacity: {
      UpdateCapacity(queue, reinterpret_cast<UpdateCapacityTask *>(task));
      break;
    }
  }
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
    case Method::kWrite: {
      ar << *reinterpret_cast<WriteTask*>(task);
      break;
    }
    case Method::kRead: {
      ar << *reinterpret_cast<ReadTask*>(task);
      break;
    }
    case Method::kAlloc: {
      ar << *reinterpret_cast<AllocTask*>(task);
      break;
    }
    case Method::kFree: {
      ar << *reinterpret_cast<FreeTask*>(task);
      break;
    }
    case Method::kMonitor: {
      ar << *reinterpret_cast<MonitorTask*>(task);
      break;
    }
    case Method::kUpdateCapacity: {
      ar << *reinterpret_cast<UpdateCapacityTask*>(task);
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
    case Method::kWrite: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<WriteTask>(task_ptr.p_);
      ar >> *reinterpret_cast<WriteTask*>(task_ptr.task_);
      break;
    }
    case Method::kRead: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<ReadTask>(task_ptr.p_);
      ar >> *reinterpret_cast<ReadTask*>(task_ptr.task_);
      break;
    }
    case Method::kAlloc: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<AllocTask>(task_ptr.p_);
      ar >> *reinterpret_cast<AllocTask*>(task_ptr.task_);
      break;
    }
    case Method::kFree: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<FreeTask>(task_ptr.p_);
      ar >> *reinterpret_cast<FreeTask*>(task_ptr.task_);
      break;
    }
    case Method::kMonitor: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<MonitorTask>(task_ptr.p_);
      ar >> *reinterpret_cast<MonitorTask*>(task_ptr.task_);
      break;
    }
    case Method::kUpdateCapacity: {
      task_ptr.task_ = LABSTOR_CLIENT->NewEmptyTask<UpdateCapacityTask>(task_ptr.p_);
      ar >> *reinterpret_cast<UpdateCapacityTask*>(task_ptr.task_);
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
    case Method::kWrite: {
      ar << *reinterpret_cast<WriteTask*>(task);
      break;
    }
    case Method::kRead: {
      ar << *reinterpret_cast<ReadTask*>(task);
      break;
    }
    case Method::kAlloc: {
      ar << *reinterpret_cast<AllocTask*>(task);
      break;
    }
    case Method::kFree: {
      ar << *reinterpret_cast<FreeTask*>(task);
      break;
    }
    case Method::kMonitor: {
      ar << *reinterpret_cast<MonitorTask*>(task);
      break;
    }
    case Method::kUpdateCapacity: {
      ar << *reinterpret_cast<UpdateCapacityTask*>(task);
      break;
    }
  }
  return ar.Get();
}
/** Deserialize a task when returning from remote queue */
void LoadEnd(u32 method, BinaryInputArchive<false> &ar, Task *task) override {
  switch (method) {
    case Method::kConstruct: {
      ar >> *reinterpret_cast<ConstructTask*>(task);
      break;
    }
    case Method::kDestruct: {
      ar >> *reinterpret_cast<DestructTask*>(task);
      break;
    }
    case Method::kWrite: {
      ar >> *reinterpret_cast<WriteTask*>(task);
      break;
    }
    case Method::kRead: {
      ar >> *reinterpret_cast<ReadTask*>(task);
      break;
    }
    case Method::kAlloc: {
      ar >> *reinterpret_cast<AllocTask*>(task);
      break;
    }
    case Method::kFree: {
      ar >> *reinterpret_cast<FreeTask*>(task);
      break;
    }
    case Method::kMonitor: {
      ar >> *reinterpret_cast<MonitorTask*>(task);
      break;
    }
    case Method::kUpdateCapacity: {
      ar >> *reinterpret_cast<UpdateCapacityTask*>(task);
      break;
    }
  }
}

#endif  // LABSTOR_BDEV_METHODS_H_