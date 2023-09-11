//
// Created by lukemartinlogan on 9/5/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_NETWORK_LOCAL_SERIALIZE_H_
#define LABSTOR_INCLUDE_LABSTOR_NETWORK_LOCAL_SERIALIZE_H_

#include "hermes_shm/data_structures/data_structure.h"

namespace labstor {

/** A class for serializing simple objects into private memory */
class LocalSerialize {
 public:
  hshm::charbuf &data_;
 public:
  LocalSerialize(hshm::charbuf &data) : data_(data) {
    data_.resize(0);
  }

  /** left shift operator */
  template<typename T>
  HSHM_ALWAYS_INLINE
  LocalSerialize& operator<<(const T &obj) {
    if constexpr(std::is_arithmetic<T>::value) {
      size_t size = sizeof(T);
      size_t off = data_.size();
      data_.resize(off + size);
      memcpy(data_.data() + off, &obj, size);
    } else if constexpr (std::is_same<T, std::string>::value || std::is_same<T, hshm::charbuf>::value) {
      size_t size = obj.size();
      size_t off = data_.size();
      data_.resize(off + size);
      memcpy(data_.data() + off, obj.data(), size);
    } else {
      throw std::runtime_error("Cannot serialize object");
    }
    return *this;
  }
};

}  // namespace labstor

#endif  // LABSTOR_INCLUDE_LABSTOR_NETWORK_LOCAL_SERIALIZE_H_
