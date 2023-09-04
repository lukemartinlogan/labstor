//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "ram_bdev/ram_bdev.h"
#include "hermes/slab_allocator.h"

namespace hermes::ram_bdev {

class Server : public TaskLib {
 public:
  SlabAllocator alloc_;
  char *mem_ptr_;

 public:
  void Construct(MultiQueue *queue, ConstructTask *task) {
    DeviceInfo &dev_info = task->info_;
    alloc_.Init(id_, dev_info.capacity_, dev_info.slab_sizes_);
    mem_ptr_ = (char*)malloc(dev_info.capacity_);
    HILOG(kInfo, "Created {} at {} of size {}",
          dev_info.dev_name_, dev_info.mount_point_, dev_info.capacity_);
    task->SetComplete();
  }

  void Destruct(MultiQueue *queue, DestructTask *task) {
    free(mem_ptr_);
    task->SetComplete();
  }

  void Alloc(MultiQueue *queue, AllocTask *task) {
    HILOG(kDebug, "Allocating {} bytes (RAM)", task->size_);
    alloc_.Allocate(task->size_, *task->buffers_, task->alloc_size_);
    HILOG(kDebug, "Allocated {} bytes (RAM)", task->alloc_size_);
    task->SetComplete();
  }

  void Free(MultiQueue *queue, FreeTask *task) {
    alloc_.Free(task->buffers_);
    task->SetComplete();
  }

  void Write(MultiQueue *queue, WriteTask *task) {
    memcpy(mem_ptr_ + task->disk_off_, task->buf_, task->size_);
    task->SetComplete();
  }

  void Read(MultiQueue *queue, ReadTask *task) {
    memcpy(task->buf_, mem_ptr_ + task->disk_off_, task->size_);
    task->SetComplete();
  }

  void Monitor(MultiQueue *queue, MonitorTask *task) {
  }

  void UpdateCapacity(MultiQueue *queue, UpdateCapacityTask *task) {
    task->SetComplete();
  }

 public:
#include "bdev/bdev_lib_exec.h"
};

}  // namespace labstor

LABSTOR_TASK_CC(hermes::ram_bdev::Server, "ram_bdev");
