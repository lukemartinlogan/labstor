//
// Created by lukemartinlogan on 9/7/21.
//

#include <memory>
#include "labstor/types/basics.h"
#include <sys/sysinfo.h>
#include "labstor/client/ipc_manager.h"

void labstor::IPCManager::Connect() {
    AUTO_TRACE("")
    ssize_t ret;
    int serverfd;
    struct sockaddr_un client_addr;
    struct sockaddr_un server_addr;
    void *region;

    TRACEPOINT("IS CONNECTED?")

    if(IsConnected()) {
        return;
    }

    TRACEPOINT("NOT CONNECTED")

    //Get our pid
    pid_ = getpid();

    //Create UDP socket
    TRACEPOINT("CREATE UDP SOCKET")
    memset(&client_addr, 0x0, sizeof(struct sockaddr_un));
    client_addr.sun_family = AF_UNIX;
    serverfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverfd == -1) {
        throw UNIX_SOCKET_FAILED.format(strerror(errno));
    }
    if (bind(serverfd, (struct sockaddr *) &client_addr, sizeof(struct sockaddr_un)) == -1) {
        close(serverfd);
        throw UNIX_BIND_FAILED.format(strerror(errno));
    }

    //Set server address & connect
    TRACEPOINT("Set server address and connect")
    memset(&server_addr, 0x0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, TRUSTED_SERVER_PATH, sizeof(server_addr.sun_path)-1);
    if (connect(serverfd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_un)) == -1) {
        close(serverfd);
        throw UNIX_CONNECT_FAILED.format(strerror(errno));
    }
    serversock_.SetFd(serverfd);

    //Receive SHMEM region
    TRACEPOINT("Get SHMEM region")
    labstor::ipc::setup_reply reply;
    serversock_.RecvMSG(&reply, sizeof(reply));

    //Mark as connected
    is_connected_ = true;
}
