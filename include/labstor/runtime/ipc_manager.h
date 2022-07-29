//
// Created by lukemartinlogan on 7/28/22.
//

#ifndef LABSTOR_IPC_MANAGER_H
#define LABSTOR_IPC_MANAGER_H

namespace labstor::Runtime {

class IPCManager {
private:
    int fd_;
public:
    void SetRuntimeFd(int fd) { fd_ = fd; };
    int GetRuntimeFd() { return fd_; };

    void RegisterClient();
};

}

#endif //LABSTOR_IPC_MANAGER_H
