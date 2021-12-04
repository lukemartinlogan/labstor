//
// Created by lukemartinlogan on 11/22/21.
//

#ifndef LABSTOR_INT_MAP_int_PerProcessIPC_H
#define LABSTOR_INT_MAP_int_PerProcessIPC_H

#include <labstor/types/data_structures/unordered_map/constants.h>
#include <labstor/userspace/util/errors.h>
#include <labstor/types/shmem_type.h>

struct labstor_int_PerProcessIPC_bucket {
    int key_;
    labstor::Server::PerProcessIPC* value_;
};

static inline void labstor_int_PerProcessIPC_bucket_Init(labstor_int_PerProcessIPC_bucket *bucket, int key, labstor::Server::PerProcessIPC* value) {
    bucket->key_ = key;
    bucket->value_ = value;
}
static inline labstor::Server::PerProcessIPC* labstor_int_PerProcessIPC_bucket_GetValue(labstor_int_PerProcessIPC_bucket *bucket, void *region) {
    return bucket->value_;
}
static inline int labstor_int_PerProcessIPC_bucket_GetKey(labstor_int_PerProcessIPC_bucket *bucket, void *region) {
    return bucket->key_;
}
static inline labstor::Server::PerProcessIPC* labstor_int_PerProcessIPC_bucket_GetAtomicValue(labstor_int_PerProcessIPC_bucket *bucket) {
    return bucket->value_;
}
static inline int labstor_int_PerProcessIPC_bucket_GetAtomicKey(labstor_int_PerProcessIPC_bucket *bucket) {
    return bucket->key_;
}
static inline int* labstor_int_PerProcessIPC_bucket_GetAtomicKeyRef(labstor_int_PerProcessIPC_bucket *bucket) {
    return &bucket->key_;
}
static inline uint32_t labstor_int_PerProcessIPC_bucket_hash(const int key, const void *region) {
    return key;
}
static inline bool labstor_int_PerProcessIPC_bucket_IsMarked(labstor_int_PerProcessIPC_bucket *bucket) {
    return labstor_int_PerProcessIPC_bucket_GetAtomicKey(bucket) & null1_mark;
}
static inline bool labstor_int_PerProcessIPC_bucket_IsNull(labstor_int_PerProcessIPC_bucket *bucket) {
    return labstor_int_PerProcessIPC_bucket_GetAtomicKey(bucket) == null1_null;
}
static inline int labstor_int_PerProcessIPC_bucket_GetMarkedAtomicKey(labstor_int_PerProcessIPC_bucket *bucket) {
    return labstor_int_PerProcessIPC_bucket_GetAtomicKey(bucket) | null1_mark;
}
static inline int labstor_int_PerProcessIPC_bucket_NullKey() {
    return null1_null;
}
static inline bool labstor_int_PerProcessIPC_bucket_KeyCompare(int key1, int key2) {
    return key1==key2;
}

#include <labstor/types/data_structures/unordered_map/shmem_unordered_map_int_PerProcessIPC_impl.h>

namespace labstor::ipc {
class int_map_int_PerProcessIPC : public unordered_map_int_PerProcessIPC {
public:
    inline bool Set(int key, labstor::Server::PerProcessIPC* value) {
        labstor_int_PerProcessIPC_bucket bucket;
        labstor_int_PerProcessIPC_bucket_Init(&bucket, key, value);
        return unordered_map_int_PerProcessIPC::Set(bucket);
    }
};

}

#endif //LABSTOR_INT_MAP_int_PerProcessIPC_H
