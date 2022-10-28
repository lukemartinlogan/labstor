/*
 * Copyright (C) 2022  SCS Lab <scslab@iit.edu>,
 * Luke Logan <llogan@hawk.iit.edu>,
 * Jaime Cernuda Garcia <jcernudagarcia@hawk.iit.edu>
 * Jay Lofstead <gflofst@sandia.gov>,
 * Anthony Kougkas <akougkas@iit.edu>,
 * Xian-He Sun <sun@iit.edu>
 *
 * This file is part of LabStor
 *
 * LabStor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef LABSTOR_DAEMON_H
#define LABSTOR_DAEMON_H

#include <labstor/util/errors.h>
#include <labstor/thread/thread_factory.h>
#include <functional>

#include <sys/sysinfo.h>
#include <sched.h>
#include <thread>
#include <future>

namespace labstor {

class Daemon {
 private:
  ThreadType type_;
  std::unique_ptr<Thread> thread_;
  bool stop_;

 public:
  explicit Daemon(ThreadType type) : stop_(false), type_(type) {}

  void Start() {
    _Init();
    auto bind = std::bind(&Daemon::DoWork, this);
    thread_ = ThreadFactory(type_, bind).Get();
  }

  void Pause() { thread_->Pause();  }
  void Resume() { thread_->Resume(); }
  void Wait() { thread_->Join(); }
  void Stop() { stop_ = true; }

 protected:
  void DoWork() {
    while (!stop_) {
      _DoWork();
    }
  }
  virtual void _Init() = 0;
  virtual void _DoWork() = 0;
};

}

#endif //LABSTOR_DAEMON_H
