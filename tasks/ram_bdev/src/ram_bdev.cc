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
      case Method::kFree: {
        Free(queue, reinterpret_cast<FreeTask *>(task));
        break;
      }
    }
  }

  void Construct(MultiQueue *queue, ConstructTask *task) {
    DeviceInfo &dev_info = *task->info_;
    alloc_.Init(id_, dev_info.capacity_, dev_info.slab_sizes_);
    mem_ptr_ = (char*)malloc(dev_info.capacity_);
    task->SetComplete();
  }

  void Destruct(MultiQueue *queue, DestructTask *task) {
    free(mem_ptr_);
    task->SetComplete();
  }

  void Alloc(MultiQueue *queue, AllocTask *task) {
    alloc_.Allocate(task->size_, task->buffers_, task->alloc_size_);
    task->SetComplete();
  }

  void Free(MultiQueue *queue, FreeTask *task) {
    alloc_.Free(*task->buffers_);
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
};

}  // namespace labstor

LABSTOR_TASK_CC(hermes::ram_bdev::Server, "ram_bdev");
