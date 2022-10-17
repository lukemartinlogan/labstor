//
// Created by lukemartinlogan on 8/1/21.
//

#ifndef SCS_SINGLETON_H
#define SCS_SINGLETON_H

#include <memory>

namespace scs {

template<typename T>
class Singleton {
 private:
  static std::unique_ptr<T> obj_;
 public:
  Singleton() = default;
  static T* GetInstance() {
    if(!obj_) { obj_ = std::make_unique<T>(); }
    return obj_.get();
  }
};

}

#endif //SCS_SINGLETON_H
