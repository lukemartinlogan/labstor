//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "posix_bdev/posix_bdev.h"
#include "hermes/slab_allocator.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

namespace hermes::posix_bdev {

class Server : public TaskLib {
 public:
  SlabAllocator alloc_;
  char *mem_ptr_;
  int fd_;

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
      case Method::kAlloc: {
        Alloc(queue, reinterpret_cast<AllocTask *>(task));
        break;
      }
      case Method::kFree: {
        Free(queue, reinterpret_cast<FreeTask *>(task));
        break;
      }
    }
  }

  void Construct(MultiQueue *queue, ConstructTask *task) {
    id_ = task->id_;
    DeviceInfo &dev_info = *task->info_;
    alloc_.Init(id_, dev_info.capacity_, dev_info.slab_sizes_);
    std::string text = dev_info.mount_dir_ +
        "/" + "slab_" + dev_info.dev_name_;
    auto canon = stdfs::weakly_canonical(text).string();
    dev_info.mount_point_ = canon;
    fd_ = open(dev_info.mount_point_.c_str(),
                  O_TRUNC | O_CREAT, 0666);
    if (fd_ < 0) {
      HELOG(kError, "Failed to open file: {}", dev_info.mount_point_);
    }
    task->SetComplete();
  }

  void Destruct(MultiQueue *queue, DestructTask *task) {
    free(mem_ptr_);
    task->SetComplete();
  }

  void Alloc(MultiQueue *queue, AllocTask *task) {
    alloc_.Allocate(task->size_, *task->buffers_, task->alloc_size_);
    task->SetComplete();
  }

  void Free(MultiQueue *queue, FreeTask *task) {
    alloc_.Free(*task->buffers_);
    task->SetComplete();
  }

  void Write(MultiQueue *queue, WriteTask *task) {
    size_t count = pwrite(fd_, task->buf_, task->size_, (off_t)task->disk_off_);
    if (count != task->size_) {
      HELOG(kError, "BORG: wrote {} bytes, but expected {}",
            count, task->size_);
    }
    task->SetComplete();
  }

  void Read(MultiQueue *queue, ReadTask *task) {
    memcpy(task->buf_, mem_ptr_ + task->disk_off_, task->size_);
    size_t count = pread(fd_, task->buf_, task->size_, (off_t)task->disk_off_);
    if (count != task->size_) {
      HELOG(kError, "BORG: read {} bytes, but expected {}",
            count, task->size_);
    }
    task->SetComplete();
  }
};

}  // namespace labstor

LABSTOR_TASK_CC(hermes::posix_bdev::Server, "posix_bdev");
