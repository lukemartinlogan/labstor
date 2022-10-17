//
// Created by lukemartinlogan on 10/16/22.
//

#ifndef LABSTOR_INCLUDE_LABSTOR_SYSINFO_INFO_H_
#define LABSTOR_INCLUDE_LABSTOR_SYSINFO_INFO_H_

#include <unistd.h>
#include <sys/sysinfo.h>

namespace labstor {

struct SystemInfo {
  int pid_;
  int ncpu_;

  SystemInfo() {
    pid_ = getpid();
    ncpu_ = get_nprocs_conf();
  }
};

}

#endif //LABSTOR_INCLUDE_LABSTOR_SYSINFO_INFO_H_
