//
// Created by lukemartinlogan on 1/7/22.
//

#ifndef LABSTOR_IO_URING_H
#define LABSTOR_IO_URING_H

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <liburing.h>

namespace labstor {

struct IOUringThread {
    char *buf_;
    int fd_;
    size_t io_offset_;
    struct io_uring ring_;
    IOUringThread(char *path, int ops_per_thread, int ops_per_batch, size_t block_size, size_t io_offset) {
        int ret;
        //Open file
        fd_ = open(path, O_DIRECT | O_CREAT | O_RDWR, 0x644);
        if(fd_ < 0) {
            printf("Could not open/create file\n");
            exit(1);
        }
        buf_ = reinterpret_cast<char*>(aligned_alloc(4096, block_size));
        memset(buf_, 0, block_size);
        io_offset_ = io_offset;
        //Create IOUring queue
        ret = io_uring_queue_init(ops_per_batch, &ring_, 0);
        if(ret < 0) {
            printf("queue_init: %s\n", strerror(-ret));
            exit(1);
        }
    }
};

class IOUringIO : public IOTest {
private:
    std::vector<IOUringThread> thread_bufs_;
public:
    void Init(char *path, size_t block_size, size_t total_size, int ops_per_batch, int nthreads, bool do_truncate) {
        IOTest::Init(block_size, total_size, ops_per_batch, nthreads);
    }
    void AIO(int op) {
        //Get per-thread data
        int tid = labstor::ThreadLocal::GetTid();
        struct IOUringThread &thread = thread_bufs_[tid];
        size_t off = 0;
        for(int i = 0; i < GetOpsPerBatch(); ++i) {
            //Get IOUring submission queue entry
            struct io_uring_sqe *sqe = io_uring_get_sqe(&thread.ring_);
            if (!sqe) {
                printf("Failed to get sqe\n");
                exit(1);
            }
            //Submit I/O request
            io_uring_prep_rw(op, sqe, thread.fd_, thread.buf_+off, block_size_, thread.io_offset_ + off);
        }

        //Wait for completion
        struct io_uring_cqe *cqe;
        for(int i = 0; i < GetOpsPerBatch(); ++i) {
            io_uring_wait_cqe(&thread.ring_, &cqe);
        }
    }

    void Read() {
        AIO(IORING_OP_READ_FIXED);
    }

    void Write() {
        AIO(IORING_OP_WRITE_FIXED);
    }
};

}

#endif //LABSTOR_IO_URING_H
