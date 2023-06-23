//
// Created by lukemartinlogan on 6/22/23.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_LABSTOR_DATA_STRUCTURE_H_
#define LABSTOR_INCLUDE_LABSTOR_LABSTOR_DATA_STRUCTURE_H_

#include <hermes_shm/data_structures/ipc/unordered_map.h>
#include <hermes_shm/data_structures/ipc/vector.h>
#include <hermes_shm/data_structures/ipc/list.h>
#include <hermes_shm/data_structures/ipc/slist.h>
#include <hermes_shm/data_structures/data_structure.h>
#include <hermes_shm/data_structures/ipc/string.h>
#include <hermes_shm/data_structures/ipc/mpsc_queue.h>
#include <hermes_shm/data_structures/containers/charbuf.h>
#include <hermes_shm/data_structures/containers/converters.h>
#include <hermes_shm/util/auto_trace.h>
#include <hermes_shm/thread/lock.h>
#include <hermes_shm/thread/thread_model_manager.h>
#include <hermes_shm/types/atomic.h>
#include "hermes_shm/util/singleton.h"

namespace labstor {

using hshm::RwLock;
using hshm::Mutex;
using hshm::bitfield32_t;
using hshm::ScopedRwReadLock;
using hshm::ScopedRwWriteLock;

typedef uint8_t u8;   /**< 8-bit unsigned integer */
typedef uint16_t u16; /**< 16-bit unsigned integer */
typedef uint32_t u32; /**< 32-bit unsigned integer */
typedef uint64_t u64; /**< 64-bit unsigned integer */
typedef int8_t i8;    /**< 8-bit signed integer */
typedef int16_t i16;  /**< 16-bit signed integer */
typedef int32_t i32;  /**< 32-bit signed integer */
typedef int64_t i64;  /**< 64-bit signed integer */
typedef float f32;    /**< 32-bit float */
typedef double f64;   /**< 64-bit float */

}

#endif //LABSTOR_INCLUDE_LABSTOR_LABSTOR_DATA_STRUCTURE_H_
