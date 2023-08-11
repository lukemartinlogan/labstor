//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "remote_queue/remote_queue.h"

namespace thallium {

/** Serialize I/O type enum */
SERIALIZE_ENUM(labstor::IoType);

}  // namespace thallium

namespace labstor::remote_queue {

class Server : public TaskLib {
 public:
  labstor::remote_queue::Client client_;

 public:
  Server() = default;

  void Run(MultiQueue *queue, u32 method, Task *task) override {
    switch (method) {
      case Method::kConstruct: {
        Construct(queue, reinterpret_cast<ConstructTask *>(task));
        break;
      }
      case Method::kDestruct: {
        Destruct(queue, reinterpret_cast<DestructTask *>(task));
        break;
      }
      case Method::kPush: {
        Push(queue, reinterpret_cast<PushTask *>(task));
        break;
      }
      case Method::kDisperse: {
        Disperse(queue, reinterpret_cast<DisperseTask *>(task));
        break;
      }
    }
  }

  /** Construct remote queue */
  void Construct(MultiQueue *queue, ConstructTask *task) {
    id_ = task->id_;
    LABSTOR_THALLIUM->RegisterRpc("RpcPushSmall", [this](const tl::request &req,
                                                         TaskStateId state_id,
                                                         std::string &params) {
      this->RpcPushSmall(req, state_id, params);
    });
    LABSTOR_THALLIUM->RegisterRpc("RpcPushBulk", [this](const tl::request &req,
                                                        TaskStateId state_id,
                                                        std::string &params,
                                                        tl::bulk &bulk,
                                                        size_t data_size,
                                                        IoType io_type) {
      this->RpcPushBulk(req, state_id, params, bulk, data_size, io_type);
    });
    task->SetComplete();
  }

  /** Destroy remote queue */
  void Destruct(MultiQueue *queue, DestructTask *task) {
    task->SetComplete();
  }

  /** Push operation called on client */
  void Push(MultiQueue *queue, PushTask *task) {
    switch (task->phase_) {
      case PushPhase::kStart: {
        auto &xfer = *task->xfer_;
        if (task->xfer_->size() == 1) {
          std::string params((char *) xfer[0].data_, xfer[0].data_size_);
          auto future = LABSTOR_THALLIUM->AsyncCall(task->domain_id_.id_,
                                                    "RpcPushSmall",
                                                    task->task_state_,
                                                    params);
          HSHM_MAKE_AR(task->tl_future_, nullptr, std::move(future));
        } else {
          std::string params((char *) xfer[1].data_, xfer[1].data_size_);
          IoType io_type = IoType::kRead;
          if (xfer[0].flags_.Any(DT_RECEIVER_READ)) {
            io_type = IoType::kWrite;
          }
          auto future = LABSTOR_THALLIUM->AsyncIoCall(task->domain_id_.id_,
                                                      "RpcPushBulk",
                                                      io_type,
                                                      (char *) xfer[0].data_,
                                                      xfer[0].data_size_,
                                                      task->task_state_,
                                                      params);
          HSHM_MAKE_AR(task->tl_future_, nullptr, std::move(future));
        }
        task->phase_ = PushPhase::kWait;
      }
      case PushPhase::kWait: {
        if (LABSTOR_THALLIUM->IsDone(*task->tl_future_)) {
          std::string ret = LABSTOR_THALLIUM->Wait<std::string>(*task->tl_future_);
          std::vector<DataTransfer> xfer(1);
          xfer[0].data_ = ret.data();
          xfer[0].data_size_ = ret.size();
          BinaryInputArchive ar(xfer);
          TaskPointer task_ptr(task);
          task->exec_->Deserialize(ar, task_ptr);
          task->SetComplete();
        }
        break;
      }
    }
    task->SetComplete();
  }

  /** Disperse operation called on client */
  void Disperse(MultiQueue *queue, DisperseTask *task) {
    task->SetComplete();
  }

 private:
  /** The RPC for processing a small message */
  void RpcPushSmall(const tl::request &req,
                    TaskStateId state_id,
                    std::string &params) {
    // Create the input data transfer object
    int ret = 0;
    std::vector<DataTransfer> xfer(1);
    xfer[0].data_ = params.data();
    xfer[0].data_size_ = params.size();
    BinaryInputArchive ar(xfer);

    // Process the message
    RpcPush(req, state_id, xfer);
  }

  /** The RPC for processing a message with data */
  void RpcPushBulk(const tl::request &req,
                   TaskStateId state_id,
                   std::string &params,
                   tl::bulk &bulk,
                   size_t data_size,
                   IoType io_type) {
    int ret = 0;
    hshm::charbuf data(data_size);
    LABSTOR_THALLIUM->IoCallServer(req, bulk, io_type, data.data(), data_size);

    // Create the input data transfer object
    std::vector<DataTransfer> xfer(2);
    xfer[0].data_ = data.data();
    xfer[0].data_size_ = data.size();
    xfer[1].data_ = params.data();
    xfer[1].data_size_ = params.size();

    // Process the message
    RpcPush(req, state_id, xfer);
  }

  /** Push operation called at the remote server */
  void RpcPush(const tl::request &req, TaskStateId state_id, std::vector<DataTransfer> &xfer) {
    BinaryInputArchive ar(xfer);

    // Deserialize task
    TaskState *exec = LABSTOR_TASK_REGISTRY->GetTaskState(state_id);
    if (exec == nullptr) {
      HELOG(kError, "Could not find the task state {}", state_id);
      req.respond(std::string());
      return;
    }
    TaskPointer task_ptr;
    exec->Deserialize(ar, task_ptr);
    auto &task = task_ptr.task_;
    auto &p = task_ptr.p_;

    // Execute task
    auto *queue = LABSTOR_QM_CLIENT->GetQueue(QueueId(state_id));
    queue->Emplace(task->lane_hash_, p);
    bool is_fire_forget = task->IsFireAndForget();

    // Get return value (should not contain data)
    if (!is_fire_forget) {
      task->Wait();
      auto out_xfer = exec->Serialize(task->method_, task);
      req.respond(std::string((char *) out_xfer[0].data_, out_xfer[0].data_size_));
    } else {
      req.respond(std::string());
    }
  }
};

}  // namespace labstor

LABSTOR_TASK_CC(labstor::remote_queue::Server, "remote_queue");
