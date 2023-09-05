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

/** Parameters for spawning async thread for thallium */
struct ThalliumTask {
  int replica_;
  PushTask *task_;
  DomainId domain_id_;
  IoType io_type_;
  char *data_;
  size_t data_size_;
  ABT_thread thread_;
  bool done_;

  /** Default constructor */
  ThalliumTask() : done_(false) {};

  /** Emplace constructor Small */
  ThalliumTask(int replica, PushTask *task, DomainId domain_id) :
      replica_(replica), task_(task), domain_id_(domain_id), done_(false) {}

  /** Emplace constructor I/O */
  ThalliumTask(int replica, PushTask *task, DomainId domain_id,
               IoType io_type, void *data, size_t data_size) :
      replica_(replica), task_(task), domain_id_(domain_id),
      io_type_(io_type), data_((char*)data), data_size_(data_size), done_(false) {}

  /** Check if the thread is finished */
  bool IsDone() {
    if (done_) {
      ABT_thread_join(thread_);
    }
    return done_;
  }
};

class Server : public TaskLib {
 public:
  labstor::remote_queue::Client client_;

 public:
  Server() = default;

  /** Construct remote queue */
  void Construct(MultiQueue *queue, ConstructTask *task) {
    LABSTOR_THALLIUM->RegisterRpc("RpcPushSmall", [this](const tl::request &req,
                                                         TaskStateId state_id,
                                                         u32 method,
                                                         std::string &params) {
      this->RpcPushSmall(req, state_id, method, params);
    });
    LABSTOR_THALLIUM->RegisterRpc("RpcPushBulk", [this](const tl::request &req,
                                                        const tl::bulk &bulk,
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

  /** Handle output from replica PUSH */
  static void HandlePushReplicaOutput(int replica, std::string &ret, PushTask *task) {
    try {
      std::vector<DataTransfer> xfer(1);
      xfer[0].data_ = ret.data();
      xfer[0].data_size_ = ret.size();
      HILOG(kDebug, "Wait got {} bytes of data (task_node={}, task_state={}, method={})",
            xfer[0].data_size_,
            task->orig_task_->task_node_,
            task->orig_task_->task_state_,
            task->orig_task_->method_);
      BinaryInputArchive<false> ar(xfer);
      task->exec_->LoadEnd(replica, task->exec_method_, ar, task->orig_task_);
    } catch (std::exception &e) {
      HILOG(kFatal, "Error LoadEnd (task_node={}, task_state={}, method={}): {}",
            task->orig_task_->task_node_,
            task->orig_task_->task_state_,
            task->orig_task_->method_,
            e.what());
    }
  }

  /** Handle finalization of PUSH replicate */
  static void HandlePushReplicaEnd(PushTask *task) {
    task->exec_->ReplicateEnd(task->orig_task_->method_, task->orig_task_);
    task->orig_task_->SetComplete();
    HILOG(kDebug, "Completing task (task_node={}, task_state={}, method={})",
          task->orig_task_->task_node_,
          task->orig_task_->task_state_,
          task->orig_task_->method_);
    task->SetComplete();
  }

  /** Push for small message */
  void ClientSmallPush(std::vector<DataTransfer> &xfer, PushTask *task) {
    task->params_ = std::string((char *) xfer[0].data_, xfer[0].data_size_);
    for (int replica = 0; replica < task->domain_ids_.size(); ++replica) {
      DomainId domain_id = task->domain_ids_[replica];
      ThalliumTask *tl_task = new ThalliumTask(replica, task, domain_id);
      tl_task->thread_ = LABSTOR_WORK_ORCHESTRATOR->SpawnAsyncThread([](void *data) {
        ThalliumTask *tl_task = (ThalliumTask *) data;
        DomainId &domain_id = tl_task->domain_id_;
        PushTask *task = tl_task->task_;
        int replica = tl_task->replica_;
        HILOG(kDebug, "(SM) Transferring {} bytes of data (task_node={}, task_state={}, method={}, from={}, to={})",
              task->params_.size(),
              task->orig_task_->task_node_,
              task->orig_task_->task_state_,
              task->orig_task_->method_,
              LABSTOR_QM_CLIENT->node_id_,
              domain_id.id_);
        std::string ret = LABSTOR_THALLIUM->SyncCall<std::string>(domain_id.id_,
                                                                  "RpcPushSmall",
                                                                  task->exec_->id_,
                                                                  task->exec_method_,
                                                                  task->params_);
        HandlePushReplicaOutput(replica, ret, task);
        tl_task->done_ = true;
      }, tl_task);
      task->tl_future_.emplace_back(tl_task);


//              tl::async_response future = LABSTOR_THALLIUM->AsyncCall(domain_id.id_,
//                                                                      "RpcPushSmall",
//                                                                      task->exec_->id_,
//                                                                      task->exec_method_,
//                                                                      params);
//              task->tl_future_.emplace_back(std::move(future));
    }
  }

  /** Push for I/O message */
  void ClientIoPush(std::vector<DataTransfer> &xfer, PushTask *task) {
    task->params_ = std::string((char *) xfer[1].data_, xfer[1].data_size_);
    IoType io_type = IoType::kRead;
    if (xfer[0].flags_.Any(DT_RECEIVER_READ)) {
      io_type = IoType::kWrite;
    }
    for (int replica = 0; replica < task->domain_ids_.size(); ++replica) {
      DomainId domain_id = task->domain_ids_[replica];
      ThalliumTask *tl_task = new ThalliumTask(
          replica, task, domain_id, io_type,
          xfer[0].data_, xfer[0].data_size_);
      tl_task->thread_ = LABSTOR_WORK_ORCHESTRATOR->SpawnAsyncThread([](void *data) {
        ThalliumTask *tl_task = (ThalliumTask *) data;
        DomainId &domain_id = tl_task->domain_id_;
        PushTask *task = tl_task->task_;
        int replica = tl_task->replica_;
        IoType &io_type = tl_task->io_type_;
        HILOG(kDebug, "(IO) Transferring {} bytes of data (task_node={}, task_state={}, method={}, from={}, to={}, type={})",
              tl_task->data_size_,
              task->orig_task_->task_node_,
              task->orig_task_->task_state_,
              task->orig_task_->method_,
              LABSTOR_QM_CLIENT->node_id_,
              domain_id.id_,
              static_cast<int>(io_type));
        std::string ret = LABSTOR_THALLIUM->SyncIoCall<std::string>(domain_id.id_,
                                                                    "RpcPushBulk",
                                                                    io_type,
                                                                    tl_task->data_,
                                                                    tl_task->data_size_,
                                                                    task->exec_->id_,
                                                                    task->exec_method_,
                                                                    task->params_,
                                                                    tl_task->data_size_,
                                                                    io_type);
        HandlePushReplicaOutput(replica, ret, task);
        tl_task->done_ = true;
      }, tl_task);
      task->tl_future_.emplace_back(tl_task);

//              tl::async_response future = LABSTOR_THALLIUM->AsyncIoCall(domain_id.id_,
//                                                                        "RpcPushBulk",
//                                                                        io_type,
//                                                                        (char *) xfer[0].data_,
//                                                                        xfer[0].data_size_,
//                                                                        task->exec_->id_,
//                                                                        task->exec_method_,
//                                                                        params,
//                                                                        xfer[0].data_size_,
//                                                                        io_type);
//              task->tl_future_.emplace_back(std::move(future));
    }
  }

  /** Wait for client to finish message */
  void ClientWaitForMessage(PushTask *task) {
    for (; task->replica_ < task->tl_future_.size(); ++task->replica_) {
//      tl::async_response &future = task->tl_future_[task->replica_];
//      if (!LABSTOR_THALLIUM->IsDone(future)) {
//        return;
//      }
//      std::string ret = LABSTOR_THALLIUM->Wait<std::string>(future);
//      HandlePushReplicaOutput(task->replica_, ret, task);
      ThalliumTask *tl_task = (ThalliumTask *) task->tl_future_[task->replica_];
      if (!tl_task->IsDone()) {
        return;
      }
      free(tl_task);
    }
    HandlePushReplicaEnd(task);
  }

  /** Push operation called on client */
  void Push(MultiQueue *queue, PushTask *task) {
    switch (task->phase_) {
      case PushPhase::kStart: {
        std::vector<DataTransfer> &xfer = task->xfer_;
        task->tl_future_.reserve(task->domain_ids_.size());
        switch (task->xfer_.size()) {
          case 1: {
            ClientSmallPush(xfer, task);
            break;
          }
          case 2: {
            ClientIoPush(xfer, task);
            break;
          }
          default: {
            HELOG(kFatal, "The task {}/{} does not support remote calls", task->task_state_, task->method_);
          }
        }
        task->phase_ = PushPhase::kWait;
      }
      case PushPhase::kWait: {
        ClientWaitForMessage(task);
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
    HILOG(kDebug, "Received small message of size {} "
                  "(task_state={}, method={})", xfer[0].data_size_, state_id, method);

    // Process the message
    RpcPush(req, state_id, method, xfer);
  }

  /** The RPC for processing a message with data */
  void RpcPushBulk(const tl::request &req,
                   TaskStateId state_id,
                   u32 method,
                   std::string &params,
                   const tl::bulk &bulk,
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

    HILOG(kDebug, "Received large message of size {} "
                  "(task_state={}, method={})", xfer[0].data_size_, state_id, method);

    // Process the message
    RpcPush(req, state_id, method, xfer);
  }

  /** Push operation called at the remote server */
  void RpcPush(const tl::request &req,
               const TaskStateId &state_id,
               u32 method,
               std::vector<DataTransfer> &xfer) {
    try {
      size_t data_size = xfer[0].data_size_;
      BinaryInputArchive<true> ar(xfer);

      // Deserialize task
      TaskState *exec = LABSTOR_TASK_REGISTRY->GetTaskState(state_id);
      if (exec == nullptr) {
        HELOG(kFatal, "(node {}) Could not find the task state {}",
              LABSTOR_QM_CLIENT->node_id_, state_id);
        req.respond(std::string());
        return;
      } else {
        HILOG(kDebug, "(node {}) Found task state {}",
              LABSTOR_QM_CLIENT->node_id_,
              state_id);
      }
      TaskPointer task_ptr = exec->LoadStart(method, ar);
      auto &orig_task = task_ptr.task_;
      auto &p = task_ptr.p_;
      orig_task->domain_id_ = DomainId::GetNode(LABSTOR_QM_CLIENT->node_id_);

      // Execute task
      auto *queue = LABSTOR_QM_CLIENT->GetQueue(QueueId(state_id));
      bool is_fire_forget = orig_task->IsFireAndForget();
      if (is_fire_forget) {
        orig_task->UnsetFireAndForget();
      }
      queue->Emplace(orig_task->lane_hash_, p);
      HILOG(kDebug, "(node {}) Executing task (task_node={}, task_state={}/{}, state_name={}, method={}, f&f={}, size={})",
            LABSTOR_QM_CLIENT->node_id_,
            orig_task->task_node_,
            orig_task->task_state_,
            state_id,
            exec->name_,
            method,
            is_fire_forget,
            data_size);

      // Get return value (should not contain data)
      // if (!is_fire_forget) {
        try {
          orig_task->Wait<1>();
          BinaryOutputArchive<false> ar(DomainId::GetNode(LABSTOR_QM_CLIENT->node_id_));
          auto out_xfer = exec->SaveEnd(method, ar, orig_task);
          LABSTOR_CLIENT->DelTask(orig_task);
          HILOG(kDebug, "Returning {} bytes of data (task_node={}, task_state={}/{}, method={}, f&f={})",
                out_xfer[0].data_size_,
                orig_task->task_node_,
                orig_task->task_state_,
                state_id,
                method,
                is_fire_forget);
          req.respond(std::string((char *) out_xfer[0].data_, out_xfer[0].data_size_));
        } catch (std::exception &e) {
          HILOG(kDebug, "SaveEnd failed (task_node={}, task_state={}/{}, method={}, f&f={}): {}",
                orig_task->task_node_,
                orig_task->task_state_,
                state_id,
                method,
                is_fire_forget,
                e.what());
        }
//      } else {
//        req.respond(std::string());
//      }
    } catch (std::exception &e) {
      HELOG(kFatal, "LoadStart {}/{}: {}", state_id, method, e.what());
    }
  }

 public:
#include "remote_queue/remote_queue_lib_exec.h"
};
}  // namespace labstor

LABSTOR_TASK_CC(labstor::remote_queue::Server, "remote_queue");
