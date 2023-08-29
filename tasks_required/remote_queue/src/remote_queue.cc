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

  /** Construct remote queue */
  void Construct(MultiQueue *queue, ConstructTask *task) {
    id_ = task->id_;
    LABSTOR_THALLIUM->RegisterRpc("RpcPushSmall", [this](const tl::request &req,
                                                         TaskStateId state_id,
                                                         u32 method,
                                                         std::string &params) {
      this->RpcPushSmall(req, state_id, method, params);
    });
    LABSTOR_THALLIUM->RegisterRpc("RpcPushBulk", [this](const tl::request &req,
                                                        tl::bulk &bulk,
                                                        TaskStateId state_id,
                                                        u32 method,
                                                        std::string &params,
                                                        size_t data_size,
                                                        IoType io_type) {
      this->RpcPushBulk(req, state_id, method, params, bulk, data_size, io_type);
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
        auto &xfer = task->xfer_;
        task->tl_future_.reserve(task->domain_ids_.size());
        switch (task->xfer_.size()) {
          case 1: {
            HILOG(kInfo, "Transferring {} of {} bytes", task->task_node_, xfer[0].data_size_);
            std::string params((char *) xfer[0].data_, xfer[0].data_size_);
            for (DomainId domain_id : task->domain_ids_) {
              tl::async_response future = LABSTOR_THALLIUM->AsyncCall(domain_id.id_,
                                                                      "RpcPushSmall",
                                                                      task->exec_->id_,
                                                                      task->exec_method_,
                                                                      params);
              task->tl_future_.emplace_back(std::move(future));
            }
            break;
          }
          case 2: {
            HILOG(kInfo, "Transferring {} of {} bytes", task->task_node_, xfer[0].data_size_);
            std::string params((char *) xfer[1].data_, xfer[1].data_size_);
            IoType io_type = IoType::kRead;
            if (xfer[0].flags_.Any(DT_RECEIVER_READ)) {
              io_type = IoType::kWrite;
            }
            for (int replica = 0; replica < task->domain_ids_.size(); ++replica) {
              DomainId domain_id = task->domain_ids_[replica];
              tl::async_response future = LABSTOR_THALLIUM->AsyncIoCall(domain_id.id_,
                                                                        "RpcPushBulk",
                                                                        io_type,
                                                                        (char *) xfer[0].data_,
                                                                        xfer[0].data_size_,
                                                                        task->exec_->id_,
                                                                        task->exec_method_,
                                                                        params,
                                                                        xfer[0].data_size_,
                                                                        io_type);
              task->tl_future_.emplace_back(std::move(future));
            }
            break;
          }
          default: {
            HELOG(kFatal, "The task {}/{} does not support remote calls", task->task_state_, task->method_);
          }
        }
        task->phase_ = PushPhase::kWait;
      }
      case PushPhase::kWait: {
        for (; task->replica_ < task->tl_future_.size(); ++task->replica_) {
          tl::async_response &future = task->tl_future_[task->replica_];
          if (!LABSTOR_THALLIUM->IsDone(future)) {
            return;
          }
          try {
            std::string ret = LABSTOR_THALLIUM->Wait<std::string>(future);
            std::vector<DataTransfer> xfer(1);
            xfer[0].data_ = ret.data();
            xfer[0].data_size_ = ret.size();
            HILOG(kInfo, "Wait ({}) got {} bytes of data", task->task_node_, xfer[0].data_size_);
            BinaryInputArchive<false> ar(xfer);
            task->exec_->LoadEnd(task->replica_, task->exec_method_, ar, task->orig_task_);
          } catch (std::exception &e) {
            HELOG(kFatal, "ERROR LoadEnd ({}): {}", task->task_node_, e.what());
          }
        }
        task->exec_->ReplicateEnd(task->orig_task_->method_, task->orig_task_);
        task->SetComplete();
        task->orig_task_->SetComplete();
        HILOG(kInfo, "Completing task {}", task->task_node_)
      }
    }
  }

 private:
  /** The RPC for processing a small message */
  void RpcPushSmall(const tl::request &req,
                    TaskStateId state_id,
                    u32 method,
                    std::string &params) {
    // Create the input data transfer object
    std::vector<DataTransfer> xfer(1);
    xfer[0].data_ = params.data();
    xfer[0].data_size_ = params.size();
    HILOG(kInfo, "Received small message of {} bytes", xfer[0].data_size_);

    // Process the message
    RpcPush(req, state_id, method, xfer);
  }

  /** The RPC for processing a message with data */
  void RpcPushBulk(const tl::request &req,
                   TaskStateId state_id,
                   u32 method,
                   std::string &params,
                   tl::bulk &bulk,
                   size_t data_size,
                   IoType io_type) {
    hshm::charbuf data(data_size);
    LABSTOR_THALLIUM->IoCallServer(req, bulk, io_type, data.data(), data_size);

    // Create the input data transfer object
    std::vector<DataTransfer> xfer(2);
    xfer[0].data_ = data.data();
    xfer[0].data_size_ = data.size();
    xfer[1].data_ = params.data();
    xfer[1].data_size_ = params.size();

    // Process the message
    RpcPush(req, state_id, method, xfer);
  }

  /** Push operation called at the remote server */
  void RpcPush(const tl::request &req,
               const TaskStateId &state_id,
               u32 method,
               std::vector<DataTransfer> &xfer) {
    try {
      BinaryInputArchive<true> ar(xfer);

      // Deserialize task
      TaskState *exec = LABSTOR_TASK_REGISTRY->GetTaskState(state_id);
      if (exec == nullptr) {
        HELOG(kError, "Could not find the task state {}", state_id);
        req.respond(std::string());
        return;
      }
      TaskPointer task_ptr = exec->LoadStart(method, ar);
      auto &orig_task = task_ptr.task_;
      auto &p = task_ptr.p_;
      orig_task->domain_id_ = DomainId::GetLocal();

      // Execute task
      auto *queue = LABSTOR_QM_CLIENT->GetQueue(QueueId(state_id));
      queue->Emplace(orig_task->lane_hash_, p);
      bool is_fire_forget = orig_task->IsFireAndForget();

      // Get return value (should not contain data)
      if (!is_fire_forget) {
        try {
          orig_task->Wait<1>();
          BinaryOutputArchive<false> ar(DomainId::GetNode(LABSTOR_QM_CLIENT->node_id_));
          auto out_xfer = exec->SaveEnd(method, ar, orig_task);
          LABSTOR_CLIENT->DelTask(orig_task);
          HILOG(kInfo, "SaveEnd ({}): Returning {} bytes of data", orig_task->task_node_, out_xfer[0].data_size_);
          req.respond(std::string((char *) out_xfer[0].data_, out_xfer[0].data_size_));
        } catch (std::exception &e) {
          HELOG(kFatal, "SaveEnd ({}): {}", orig_task->task_node_, e.what());
        }
      } else {
        req.respond(std::string());
      }
    } catch (std::exception &e) {
      HELOG(kFatal, "LoadStart {}/{}: {}", state_id, method, e.what());
    }
  }

 public:
#include "remote_queue/remote_queue_lib_exec.h"
};
}  // namespace labstor

LABSTOR_TASK_CC(labstor::remote_queue::Server, "remote_queue");
