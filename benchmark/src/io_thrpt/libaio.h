//
// Created by lukemartinlogan on 1/7/22.
//

#ifndef LABSTOR_LIBAIO_H
#define LABSTOR_LIBAIO_H

#include "unix_file_based.h"
#include <libaio.h>

namespace labstor {

struct LibAIOThread : public UnixFileBasedIOThread {
    io_context_t ctx_;
    struct iocb *cbs_;
    struct io_event *events_;
    LibAIOThread(char *path, int ops_per_batch, size_t block_size) : UnixFileBasedIOThread(path, block_size) {
        cbs_ = reinterpret_cast<struct iocb*>(calloc(ops_per_batch, sizeof(struct iocb)));
        events_ = reinterpret_cast<struct io_event*>(calloc(ops_per_batch, sizeof(struct io_event)));
        io_queue_init(ops_per_batch, &ctx_);
    }
};

class LibAIO : public UnixFileBasedIOTest {
private:
    std::vector<LibAIOThread> thread_bufs_;
public:
    void Init(char *path, bool do_truncate, labstor::Generator *generator) {
        UnixFileBasedIOTest::Init(path, do_truncate, generator);
        //Store per-thread data
        for(int i = 0; i < GetNumThreads(); ++i) {
            thread_bufs_.emplace_back(path, GetOpsPerBatch(), GetBlockSizeBytes());
        }
    }
    void AIO(int op) {
        int tid = labstor::ThreadLocal::GetTid();
        struct LibAIOThread &thread = thread_bufs_[tid];
        size_t off = 0;
        for(size_t i = 0; i < GetOpsPerBatch(); ++i) {
            struct iocb *cb = thread.cbs_+i;
            switch(op) {
                case 0:
                    io_prep_pread(cb, thread.fd_, thread.buf_ + off, GetBlockSizeBytes(), GetOffsetBytes(tid));
                    break;
                case 1:
                    io_prep_pwrite(cb, thread.fd_, thread.buf_ + off, GetBlockSizeBytes(), GetOffsetBytes(tid));
                    break;
            }
            io_submit(thread.ctx_, 1, &cb);
            off += GetBlockSizeBytes();
        }
        int ret = io_getevents(thread.ctx_, GetOpsPerBatch(), GetOpsPerBatch(), thread.events_, NULL);
        if(ret != GetOpsPerBatch()) {
            printf("Error occurred reading events: %d\n", ret);
            exit(1);
        }
        for(int i = 0; i < GetOpsPerBatch(); ++i) {
            if(thread.events_[i].res != GetBlockSizeBytes()) {
                struct iocb *cb = thread.cbs_+i;
                printf("LibAIO[%d]: I/O failed: %s\n", i, strerror(-thread.events_[i].res));
                printf("CB: fd=%d disk_off=%lu nbytes=%lu prio=%d\n", cb->aio_fildes, cb->u.c.offset, cb->u.c.nbytes, cb->aio_reqprio);
                exit(1);
            }
        }
    }

    void Read() {
        AIO(0);
    }

    void Write() {
        AIO(1);
    }
};

}

#endif //LABSTOR_LIBAIO_H
