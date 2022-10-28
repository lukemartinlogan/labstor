//
// Created by lukemartinlogan on 10/27/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_MEMORY_ATTACHABLE_H_
#define LABSTOR_INCLUDE_LABSTOR_MEMORY_ATTACHABLE_H_

#include <string>

namespace labstor::memory {

class Attachable {
 public:
  virtual bool Create() = 0;
  virtual bool Attach() = 0;
  virtual void Detach() = 0;
  void Destroy() {
    Detach();
    _Destroy();
  }

 protected:
  virtual void _Destroy() = 0;
};

class BufferAttachable {
 public:
  virtual bool Create(void *buffer, size_t size) = 0;
  virtual bool Attach(void *buffer) = 0;
};

}  // namespace labstor::memory

#endif //LABSTOR_INCLUDE_LABSTOR_MEMORY_ATTACHABLE_H_
