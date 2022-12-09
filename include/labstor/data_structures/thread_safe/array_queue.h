//
// Created by lukemartinlogan on 11/6/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_ARRAY_QUEUE_H_
#define LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_LOCKLESS_ARRAY_QUEUE_H_

#include <labstor/data_structures/thread_unsafe/vector.h>

namespace labstor::ipc {

template<typename T>
class array_queue : public vector<T> {
 public:

};

}

#endif //LABSTOR_INCLUDE_LABSTOR_DATA_STRUCTURES_ARRAY_QUEUE_H_
