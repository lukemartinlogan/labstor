//
// Created by lukemartinlogan on 11/18/21.
//

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
