//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "remote_queue/remote_queue.h"
#include <rdma/fabric.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_cm.h>

/* Fallback for undefined OPX values */
#if FI_VERSION_LT(FI_VERSION(FI_MAJOR_VERSION, FI_MINOR_VERSION),              \
    FI_VERSION(1, 15))
#    define FI_ADDR_OPX  -1
#    define FI_PROTO_OPX -1
#endif

/* Fallback for undefined CXI values */
#if FI_VERSION_LT(FI_VERSION(FI_MAJOR_VERSION, FI_MINOR_VERSION),              \
    FI_VERSION(1, 14)) /* < 1.14 */
#    define FI_ADDR_CXI  -2
#    define FI_PROTO_CXI -2
#elif (FI_MAJOR_VERSION == 1 && FI_MINOR_VERSION == 14) /* = 1.14 */
#    define FI_ADDR_CXI  (FI_ADDR_PSMX3 + 1)
#    define FI_PROTO_CXI (FI_PROTO_PSMX3 + 1)
#elif (FI_MAJOR_VERSION == 1 && FI_MINOR_VERSION == 15 &&                      \
       FI_REVISION_VERSION < 2) /* 1.15 */
#    define FI_ADDR_CXI  (FI_ADDR_OPX + 1)
#    define FI_PROTO_CXI (FI_PROTO_OPX + 1)
#endif

/* Fallback for undefined XNET values */
#if FI_VERSION_LT(FI_VERSION(FI_MAJOR_VERSION, FI_MINOR_VERSION),              \
    FI_VERSION(1, 17))
#    define FI_PROTO_XNET -3
#endif

namespace labstor::remote_queue {

struct ProviderInfo {
  int preferred_addr_fmt_;
  int native_addr_fmt_;
  int progress_mode_;
  int endpoint_proto_;
  unsigned long long caps_;
  int misc_;

  bool IsProvider(const std::string provider, const std::string &test) {
    return provider.find(test) != std::string::npos;
  }

  ProviderInfo(const std::string &provider) {
    if (IsProvider(provider, "shm") || IsProvider(provider, "sm")) {
      preferred_addr_fmt_ = FI_ADDR_STR;
      native_addr_fmt_ = FI_ADDR_STR;
      progress_mode_ = FI_PROGRESS_MANUAL;
      endpoint_proto_ = FI_PROTO_SHM;
      caps_ = FI_SOURCE | FI_MULTI_RECV;
      // NA_OFI_HMEM
    } else if (IsProvider(provider, "sockets")) {
      preferred_addr_fmt_ = FI_SOCKADDR_IN;
      native_addr_fmt_ = FI_SOCKADDR_IN;
      progress_mode_ = FI_PROGRESS_AUTO;
      endpoint_proto_ = FI_PROTO_SOCK_TCP;
      caps_ = FI_SOURCE | FI_MULTI_RECV;
      // NA_OFI_DOM_IFACE | NA_OFI_WAIT_FD | NA_OFI_SEP | NA_OFI_DOM_SHARED
    } else if (IsProvider(provider, "tcp") || IsProvider(provider, "tcp_exp")) {
      preferred_addr_fmt_ = FI_SOCKADDR_IN;
      native_addr_fmt_ = FI_SOCKADDR_IN;
      progress_mode_ = FI_PROGRESS_AUTO;
      endpoint_proto_ = FI_PROTO_XNET;
      caps_ = FI_SOURCE | FI_MULTI_RECV;
      // NA_OFI_DOM_IFACE | NA_OFI_WAIT_FD
    } else if (IsProvider(provider, "psm2")) {
      preferred_addr_fmt_ = FI_ADDR_PSMX2;
      native_addr_fmt_ = FI_ADDR_PSMX2;
      progress_mode_ = FI_PROGRESS_MANUAL;
      endpoint_proto_ = FI_PROTO_PSMX2;
      caps_ = FI_SOURCE | FI_SOURCE_ERR | FI_MULTI_RECV;
      // NA_OFI_DOM_IFACE | NA_OFI_WAIT_FD
    } else if (IsProvider(provider, "opx")) {
      preferred_addr_fmt_ = FI_ADDR_OPX;
      native_addr_fmt_ = FI_ADDR_OPX;
      progress_mode_ = FI_PROGRESS_MANUAL;
      endpoint_proto_ = FI_PROTO_OPX;
      caps_ = 0;
      // NA_OFI_SIGNAL | NA_OFI_SEP | NA_OFI_CONTEXT2
    } else if (IsProvider(provider, "verbs")) {
      preferred_addr_fmt_ = FI_SOCKADDR_IN;
      native_addr_fmt_ = FI_SOCKADDR_IB;
      progress_mode_ = FI_PROGRESS_MANUAL;
      endpoint_proto_ = FI_PROTO_RXM;
      caps_ = FI_MULTI_RECV;
      // NA_OFI_WAIT_FD | NA_OFI_LOC_INFO | NA_OFI_HMEM
    } else if (IsProvider(provider, "gni")) {
      preferred_addr_fmt_ = FI_ADDR_GNI;
      native_addr_fmt_ = FI_ADDR_GNI;
      progress_mode_ = FI_PROGRESS_AUTO;
      endpoint_proto_ = FI_PROTO_GNI;
      caps_ = FI_SOURCE | FI_SOURCE_ERR | FI_MULTI_RECV;
      // NA_OFI_WAIT_SET | NA_OFI_SIGNAL | NA_OFI_SEP
    } else if (IsProvider(provider, "cxi")) {
      preferred_addr_fmt_ = FI_ADDR_CXI;
      native_addr_fmt_ = FI_ADDR_CXI;
      progress_mode_ = FI_PROGRESS_MANUAL;
      endpoint_proto_ = FI_PROTO_CXI;
      caps_ = FI_MULTI_RECV;
      // NA_OFI_WAIT_FD | NA_OFI_LOC_INFO | NA_OFI_HMEM
    } else {
      HELOG(kError, "Unkown provider {}", provider);
    }
  }
};

struct ConnectionInfo {
  std::vector<char> data_;
  struct fi_info* fi_;          /**< General fabric info */
  struct fi_info *hints_;       /**< Properties for creating info */
  struct fid_fabric* fabric_;   /**< Fabric ID */
  struct fid_domain* domain_;   /**< Fabric domain */
  struct fid_av *av_;           /**< Address vector */
  struct fid_pep *pep_;         /**< Passive endpoint */
  struct fid_ep* ep_;           /**< Active endpoint */
  struct fid_eq *eq_;           /**< Emission queue (RDMA) */
  struct fid_cntr *cntr_;        /**< Completion counter */
  struct fid_mr* mr_;           /**< Memory region (RDMA) */
  std::list<std::unique_ptr<ConnectionInfo>> clients_;

  char* copy_string(const std::string &str) {
    char* ret = new char[str.size() + 1];
    std::copy(str.begin(), str.end(), ret);
    ret[str.size()] = '\0';
    return ret;
  }

  int CommonInit(const std::string &provider, int port, const std::string &ip_addr) {
    int ret;

    // Allocate fabric info
    ProviderInfo prov_info(provider);
    hints_ = fi_allocinfo();
    hints_->fabric_attr->prov_name = copy_string(provider);
    std::string port_str = std::to_string(port);
    ret = fi_getinfo(FI_VERSION(1, 14),
                     ip_addr.c_str(), port_str.c_str(),
                     0, hints_, &fi_);
    if (ret) {
      HELOG(kError, "Failed to get fabric info");
      return ret;
    }

    // Open a fabric domain & initialize endpoint
    ret = fi_fabric(fi_->fabric_attr, &fabric_, NULL);
    if (ret) {
      HELOG(kError, "Failed to initialize fabric");
      return ret;
    }
    ret = fi_domain(fabric_, fi_, &domain_, NULL);
    if (ret) {
      HELOG(kError, "Failed to initialize domain");
      return ret;
    }

    // Create emission queue
    struct fi_eq_attr eq_attr = {
        .size = 0,
        .flags = 0,
        .wait_obj = FI_WAIT_UNSPEC,
        .signaling_vector = 0,
        .wait_set = NULL,
    };
    ret = fi_eq_open(fabric_, &eq_attr, &eq_, NULL);
    if (ret) {
      perror("fi_eq_open");
      return ret;
    }

    // Open completion counter
    /*
    struct fi_cntr_attr cntr_attr;
    memset(&cntr_attr, 0, sizeof(cntr_attr));
    ret = fi_cntr_open(domain_, &cntr_attr, &cntr_, NULL);
    if (ret) {
      HELOG(kError, "Failed to open completion queue")
      return ret;
    }*/
    return ret;
  }

  int ServerInit(const std::string &provider, int port, const std::string &ip_addr) {
    int ret;

    // Initialize common data structures between client & server
    ret = CommonInit(provider, port, ip_addr);
    if (ret) {
      return ret;
    }

    // Create passive endpoint
    ret = fi_passive_ep(fabric_, fi_, &pep_, NULL);
    if (ret) {
      HELOG(kError, "Failed to initialize server endpoint");
      return ret;
    }

    // Bind passive endpoint to emission queue
    ret = fi_pep_bind(pep_, &eq_->fid, 0);
    if (ret) {
      perror("fi_pep_bind(eq)");
      return ret;
    }

    // Bind passive endpoint to completion counter
    /*ret = fi_pep_bind(pep_, &cntr_->fid,
                FI_SEND | FI_RECV);
    if (ret) {
      HELOG(kError, "Failed to bind completion counter to passive endpoint");
    }*/

    // Initialize RDMA or Socket
    if (fi_->caps & FI_RMA) {
      HILOG(kInfo, "{} {} supports RDMA", ip_addr, provider);
      ServerInitRDMA();
    } else {
      HILOG(kInfo, "{} {} does NOT support RDMA", ip_addr, provider);
      ServerInitSocket(port);
    }

    // Listen for new connections
    ret = fi_listen(pep_);
    if (ret) {
      HELOG(kError, "Failed to listen for new connections");
      return ret;
    }

    return ret;
  }

  int ServerInitRDMA() {
    int ret;

    // Finish setting up RDMA region
    data_.resize(KILOBYTES(64));
    ret = fi_mr_reg(domain_, data_.data(), data_.size(),
                    FI_READ | FI_WRITE | FI_REMOTE_READ | FI_REMOTE_WRITE,
                    0, 0, 0, &mr_, NULL);
    if (ret) {
      HELOG(kError, "Failed to register memory region");
      return ret;
    }

    return ret;
  }

  int ServerInitSocket(int port) {
    return 0;
  }

  int ClientInit(const std::string &provider, int port, const std::string &ip_addr) {
    int ret;

    // Initialize common data structures
    ret = CommonInit(provider, port, ip_addr);
    if (ret) {
      return ret;
    }

    // Create an active endpoint
    ret = fi_endpoint(domain_, fi_, &ep_, NULL);
    if (ret) {
      HELOG(kError, "Failed to initialize endpoint");
      return ret;
    }

    // Bind endpoint to emission queue
    ret = fi_ep_bind(ep_, &eq_->fid, 0);
    if (ret) {
      perror("fi_pep_bind(eq)");
      return ret;
    }

    // Bind endpoint to completion counter
    /*ret = fi_ep_bind(ep_, &cntr_->fid,
                     FI_SEND | FI_RECV);
    if (ret) {
      HELOG(kError, "Failed to bind completion counter to endpoint");
    }
    // FI_SEND | FI_RECV | FI_READ | FI_WRITE | FI_REMOTE_READ | FI_REMOTE_WRITE*/

    // Initialize RDMA or Socket
    ret = fi_connect(ep_, fi_->dest_addr, NULL, 0);
    if (ret) {
      perror("fi_connect");
      return ret;
    }

    return ret;
  }

  int ServerAcceptRdma() {
    int ret;
    struct fi_eq_cm_entry entry;
    uint32_t event;

    // Check if client has connected
//    ret = fi_eq_read(eq_, &event, &entry, sizeof (entry), 0);
//    if (ret != sizeof (entry)) {
//      HILOG(kWarning, "fi_eq_read: {}", strerror(errno));
//      return (int)ret;
//    }
//    if (event != FI_CONNREQ) {
//      fprintf(stderr, "invalid event %u\n", event);
//      return -1;
//    }

    // Accept the endpoint
    ret = fi_accept(ep_, NULL, 0);
    if (ret) {
      HELOG(kError, "Failed to accept endpoint");
      return ret;
    }

    // Register the client connection
    clients_.emplace_back();
    clients_.back()->RegisterClientConn(entry, domain_);
    return ret;
  }

  int ServerAcceptTcp() {
    int ret;
    struct fi_eq_cm_entry entry;
    uint32_t event;

    // Accept the endpoint
    ret = fi_accept(ep_, NULL, 0);
    if (ret) {
      HELOG(kError, "Failed to accept endpoint");
      return ret;
    }

    // Register the client connection
    clients_.emplace_back();
    clients_.back()->RegisterClientConn(entry, domain_);
    return ret;
  }

  int RegisterClientConn(struct fi_eq_cm_entry &entry, struct fid_domain* domain) {
    int ret;
    domain_ = domain;

    // Create endpoint to the server
    ret = fi_endpoint(domain_, entry.info, &ep_, NULL);
    if (ret) {
      HELOG(kError, "Failed to create endpoint");
      return ret;
    }

    // Open emission queue
    struct fi_eq_attr eq_attr = {
        .size = 0,
        .flags = 0,
        .wait_obj = FI_WAIT_UNSPEC,
        .signaling_vector = 0,
        .wait_set = NULL,
    };
    ret = fi_eq_open(fabric_, &eq_attr, &eq_, NULL);
    if (ret) {
      HELOG(kError, "Failed to open emission queue")
      return ret;
    }
    fi_ep_bind(ep_, &eq_->fid,
               FI_SEND | FI_RECV | FI_READ | FI_WRITE | FI_REMOTE_READ | FI_REMOTE_WRITE);

    // Open completion counter
    struct fi_cntr_attr cntr_attr;
    memset(&cntr_attr, 0, sizeof(cntr_attr));
    ret = fi_cntr_open(domain_, &cntr_attr, &cntr_, NULL);
    if (ret) {
      HELOG(kError, "Failed to open completion queue")
      return ret;
    }
    fi_ep_bind(ep_, &cntr_->fid,
               FI_SEND | FI_RECV | FI_READ | FI_WRITE | FI_REMOTE_READ | FI_REMOTE_WRITE);

    // Enable the ep for
    ret = fi_enable(ep_);
    if (ret) {
      HELOG(kError, "Failed to enable endpoint");
      return ret;
    }

    return ret;
  }

  void Send() {
  }

  void Recv() {
  }
};

class Server : public TaskLib {
 public:
  std::unordered_map<DomainId, ConnectionInfo> conn_;
  ConnectionInfo my_conn_;
  ConnectionInfo my_client_conn_;
  AcceptTask *accept_;
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
      case Method::kAccept: {
        Accept(queue, reinterpret_cast<AcceptTask *>(task));
        break;
      }
      case Method::kDisperse: {
        Disperse(queue, reinterpret_cast<DisperseTask *>(task));
        break;
      }
    }
  }

  void Construct(MultiQueue *queue, ConstructTask *task) {
    id_ = task->id_;
    int my_node_id = LABSTOR_QM_CLIENT->node_id_;
    for (HostInfo &host_info : LABSTOR_RUNTIME->rpc_.hosts_) {
      if (host_info.node_id_ != LABSTOR_QM_CLIENT->node_id_) {
        /*DomainId domain_id = DomainId::GetNode(host_info.node_id_);
        conn_[domain_id] = ConnectionInfo();
        conn_[domain_id].ClientInit();*/
      } else {
        my_conn_.ServerInit(LABSTOR_RUNTIME->rpc_.protocol_,
                            LABSTOR_RUNTIME->rpc_.port_,
                            host_info.ip_addr_);
        client_.Init(id_);
        accept_ = client_.AsyncAcceptThread();
        my_client_conn_.ClientInit(LABSTOR_RUNTIME->rpc_.protocol_,
                                   LABSTOR_RUNTIME->rpc_.port_,
                                   host_info.ip_addr_);
      }
    }
    task->SetComplete();
  }

 public:
  void Destruct(MultiQueue *queue, DestructTask *task) {
    task->SetComplete();
  }

  void Accept(MultiQueue *queue, AcceptTask *task) {
    my_conn_.ServerAcceptTcp();
  }

  void Push(MultiQueue *queue, PushTask *task) {
    task->SetComplete();
  }

  void Disperse(MultiQueue *queue, DisperseTask *task) {
    task->SetComplete();
  }
};

}  // namespace labstor

LABSTOR_TASK_CC(labstor::remote_queue::Server, "remote_queue");
