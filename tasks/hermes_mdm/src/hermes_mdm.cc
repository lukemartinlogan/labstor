//
// Created by lukemartinlogan on 6/29/23.
//

#include "labstor_admin/labstor_admin.h"
#include "labstor/api/labstor_runtime.h"
#include "hermes_mdm/hermes_mdm.h"
#include "hermes_bpm/hermes_bpm.h"

namespace labstor::hermes_mdm {

/** Represents an allocated fraction of a target */
struct BufferInfo {
  TargetId tid_;        /**< The destination target */
  size_t t_slab_;          /**< The index of the slab in the target */
  size_t t_off_;        /**< Offset in the target */
  size_t t_size_;       /**< Size in the target */
  size_t blob_off_;     /**< Offset in the blob */
  size_t blob_size_;    /**< The amount of the blob being placed */

  /** Default constructor */
  BufferInfo() = default;

  /** Primary constructor */
  BufferInfo(TaskStateId tid, size_t t_off, size_t t_size,
             size_t blob_off, size_t blob_size)
      : tid_(tid), t_off_(t_off), t_size_(t_size),
        blob_off_(blob_off), blob_size_(blob_size) {}

  /** Copy constructor */
  BufferInfo(const BufferInfo &other) {
    Copy(other);
  }

  /** Move constructor */
  BufferInfo(BufferInfo &&other) {
    Copy(other);
  }

  /** Copy assignment */
  BufferInfo& operator=(const BufferInfo &other) {
    Copy(other);
    return *this;
  }

  /** Move assignment */
  BufferInfo& operator=(BufferInfo &&other) {
    Copy(other);
    return *this;
  }

  /** Performs move/copy */
  void Copy(const BufferInfo &other) {
    tid_ = other.tid_;
    t_slab_ = other.t_slab_;
    t_off_ = other.t_off_;
    t_size_ = other.t_size_;
    blob_off_ = other.blob_off_;
    blob_size_ = other.blob_size_;
  }
};

/** Data structure used to store Blob information */
struct BlobInfo {
  BlobId id_;  /**< Unique ID of the blob */
  std::string name_;  /**< Name of the blob */
  std::vector<BufferInfo> buffers_;  /**< Set of buffers */
  std::vector<TagId> tags_;  /**< Set of tags */
  size_t blob_size_;  /**< The overall size of the blob */
  float score_;  /**< The priority of this blob */
  std::atomic<u32> access_freq_;  /**< Number of times blob accessed in epoch */
  u64 last_access_;  /**< Last time blob accessed */
  std::atomic<size_t> mod_count_;   /**< The number of times blob modified */
  std::atomic<size_t> last_flush_;  /**< The last mod that was flushed */
};

/** Data structure used to store Bucket information */
struct TagInfo {
  std::string tag_id_;
  std::string name_;
  std::list<BlobId> blobs_;
  std::list<Task*> traits_;
  size_t internal_size_;
  bool owner_;
};

/** The types of I/O that can be performed (for IoCall RPC) */
enum class IoType {
  kRead,
  kWrite,
  kNone
};

/** Indicates a PUT or GET for a particular blob */
struct IoStat {
  IoType type_;
  BlobId blob_id_;
  TagId tag_id_;
  size_t blob_size_;
  int rank_;

  /** Default constructor */
  IoStat() = default;

  /** Copy constructor */
  IoStat(const IoStat &other) {
    Copy(other);
  }

  /** Copy assignment */
  IoStat& operator=(const IoStat &other) {
    if (this != &other) {
      Copy(other);
    }
    return *this;
  }

  /** Move constructor */
  IoStat(IoStat &&other) {
    Copy(other);
  }

  /** Move assignment */
  IoStat& operator=(IoStat &&other) {
    if (this != &other) {
      Copy(other);
    }
    return *this;
  }

  /** Generic copy / move */
  HSHM_ALWAYS_INLINE void Copy(const IoStat &other) {
    type_ = other.type_;
    blob_id_ = other.blob_id_;
    tag_id_ = other.tag_id_;
    blob_size_ = other.blob_size_;
    rank_ = other.rank_;
  }

  /** Serialize */
  template<class Archive>
  void save(Archive &ar) const {
    int type = static_cast<int>(type_);
    u64 ids[2] = {blob_id_.unique_, tag_id_.unique_};
    u32 nodes[2] = {blob_id_.node_id_, tag_id_.node_id_};
    ar(type, ids[0], nodes[0], ids[1], nodes[1], blob_size_, rank_);
  }

  /** Deserialize */
  template<class Archive>
  void load(Archive &ar) {
    int type;
    ar(type,
       blob_id_.unique_,
       blob_id_.node_id_,
       tag_id_.unique_,
       tag_id_.node_id_,
       blob_size_,
       rank_);
    type_ = static_cast<IoType>(type);
  }
};

/** Type name simplification for the various map types */
typedef std::unordered_map<std::string, BlobId> BLOB_ID_MAP_T;
typedef std::unordered_map<std::string, TagId> TAG_ID_MAP_T;
typedef std::unordered_map<BlobId, BlobInfo> BLOB_MAP_T;
typedef std::unordered_map<TagId, TagInfo> TAG_MAP_T;
typedef hipc::mpsc_queue<IoStat> IO_PATTERN_LOG_T;

class Server : public TaskLib {
 public:
  /**====================================
   * Maps
   * ===================================*/
  BLOB_ID_MAP_T blob_id_map_;
  TAG_ID_MAP_T tag_id_map_;
  BLOB_MAP_T blob_map_;
  TAG_MAP_T tag_map_;
  std::atomic<u64> id_alloc_;

  /**====================================
  * I/O pattern log
  * ===================================*/
  IO_PATTERN_LOG_T *io_pattern_log_;
  bool enable_io_tracing_;
  bool is_mpi_;

  /**====================================
   * Targets + devices
   * ===================================*/
  std::vector<hermes_bpm::Client> targets_;

 public:
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
      case Method::kCustom: {
        Custom(queue, reinterpret_cast<CustomTask *>(task));
        break;
      }
    }
  }

  void Construct(MultiQueue *queue, ConstructTask *task) {
    id_alloc_ = 0;
    task->SetComplete();
  }

  void Destruct(MultiQueue *queue, DestructTask *task) {
    task->SetComplete();
  }

  /** Get or create a tag */
  void GetOrCreateTag(MultiQueue *queue, GetOrCreateTagTask *task) {
    // Create unique ID for the Bucket
    TagId tag_id;
    tag_id.unique_ = id_alloc_.fetch_add(1);
    tag_id.node_id_ = LABSTOR_RUNTIME->rpc_.node_id_;
    bool exists = tag_id_map_.find(tag);

    // Emplace bucket if it does not already exist
    if (did_create) {
      HILOG(kDebug, "Creating tag for the first time: {} {}", tag_name, tag_id)
      tag_map_->emplace(tag_id);
      auto iter = tag_map_->find(tag_id);
      hipc::pair<TagId, TagInfo> &info_pair = *iter;
      TagInfo &info = info_pair.GetSecond();
      (*info.name_) = *tag_name_shm;
      info.tag_id_ = tag_id;
      info.owner_ = owner;
      info.internal_size_ = backend_size;
    } else {
      HILOG(kDebug, "Found existing tag: {}", tag_name)
      auto iter = tag_id_map_->find(*tag_name_shm);
      hipc::pair<hipc::charbuf, TagId> &id_info = (*iter);
      tag_id = id_info.GetSecond();
    }

    return {tag_id, did_create};
  }
};

}  // namespace labstor

LABSTOR_TASK_CC(labstor::hermes_mdm::Server, "hermes_mdm");
