//
// Created by lukemartinlogan on 1/19/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SMART_PTR_INPLACE_PTR_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SMART_PTR_INPLACE_PTR_H_

template<typename T>
class inplace_ptr {
  template<typename ...Args>
  inplace_ptr(typename T::header_t *header, Args&& ...args) {
    T(header, std::forward<Args>(args)...).UnsetDestructable();
  }
};

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_SMART_PTR_INPLACE_PTR_H_
