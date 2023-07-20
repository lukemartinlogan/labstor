//
// Created by lukemartinlogan on 7/19/23.
//

#ifndef LABSTOR_TASKS_HERMES_ADAPTERS_INCLUDE_HERMES_ADAPTERS_FILESYSTEM_STRUCTS_H_
#define LABSTOR_TASKS_HERMES_ADAPTERS_INCLUDE_HERMES_ADAPTERS_FILESYSTEM_STRUCTS_H_

#include <ftw.h>
#include <mpi.h>
#include <future>
#include <set>
#include <string>

#include "hermes/bucket.h"
#include "hermes/hermes.h"
#include "adapter_types.h"

#include <filesystem>

namespace hermes::adapter {

/** Put or get data directly from I/O client */
#define HERMES_IO_CLIENT_BYPASS BIT_OPT(uint32_t, 0)
/** Only put or get data from a Hermes buffer; no fallback to I/O client */
#define HERMES_IO_CLIENT_NO_FALLBACK BIT_OPT(uint32_t, 1)
/** Whether to perform seek */
#define HERMES_FS_SEEK BIT_OPT(uint32_t, 2)
/** Whether to perform create */
#define HERMES_FS_CREATE BIT_OPT(uint32_t, 3)
/** Whether in append mode */
#define HERMES_FS_APPEND BIT_OPT(uint32_t, 4)
/** Whether to perform truncate */
#define HERMES_FS_TRUNC BIT_OPT(uint32_t, 5)
/** Whether the file was found on-disk */
#define HERMES_FS_EXISTS BIT_OPT(uint32_t, 6)

/**
 * A structure to represent IO options for FS adapter.
 * For now, nothing additional than the typical FsIoOptions.
 * */
struct FsIoOptions {
  AdapterMode adapter_mode_;      /**< Current adapter mode for this obj */
  PlacementPolicy dpe_;     /**< data placement policy */
  bitfield32_t flags_;            /**< various I/O flags */
  MPI_Datatype mpi_type_; /**< MPI data type */
  int mpi_count_;         /**< The number of types */
  size_t backend_off_;    /**< Offset in the backend to begin I/O */
  size_t backend_size_;   /**< Size of I/O to perform at backend */

  /** Default constructor */
  FsIoOptions() : dpe_(PlacementPolicy::kNone),
                  flags_(),
                  mpi_type_(MPI_CHAR),
                  mpi_count_(0),
                  backend_off_(0),
                  backend_size_(0) {
    SetSeek();
  }

  /** Enable seek for this I/O */
  void SetSeek() {
    flags_.SetBits(HERMES_FS_SEEK);
  }

  /** Disable seek for this I/O */
  void UnsetSeek() {
    flags_.UnsetBits(HERMES_FS_SEEK);
  }

  /** Whether or not to perform seek in FS adapter */
  bool DoSeek() const {
    return flags_.Any(HERMES_FS_SEEK);
  }

  /** Marks the file as truncated */
  void MarkTruncated() {
    flags_.SetBits(HERMES_FS_TRUNC);
  }

  /** Whether a file is marked truncated */
  bool IsTruncated() const {
    return flags_.Any(HERMES_FS_TRUNC);
  }

  /** return IO options with \a mpi_type MPI data type */
  static FsIoOptions DataType(MPI_Datatype mpi_type, bool seek = true) {
    FsIoOptions opts;
    opts.mpi_type_ = mpi_type;
    if (!seek) { opts.UnsetSeek(); }
    return opts;
  }

  /** Return Io options with \a DPE */
  static FsIoOptions WithDpe(PlacementPolicy dpe) {
    FsIoOptions opts;
    opts.dpe_ = dpe;
    return opts;
  }
};

/** Represents an object in the I/O client (e.g., a file) */
struct File {
  AdapterType type_;     /**< Client to forward I/O request to */
  std::string filename_;  /**< Filename to read from */

  int hermes_fd_;          /**< fake file descriptor (SCRATCH MODE) */
  FILE *hermes_fh_;        /**< fake file handler (SCRATCH MODE) */
  MPI_File hermes_mpi_fh_; /**< fake MPI file handler (SCRATCH MODE) */

  bool status_;    /**< status */
  int mpi_status_; /**< MPI status */

  /** Default constructor */
  File()
      : type_(AdapterType::kNone),
        filename_(),
        hermes_fd_(-1),
        hermes_fh_(nullptr),
        hermes_mpi_fh_(nullptr),
        status_(true),
        mpi_status_(MPI_SUCCESS) {}

  /** file constructor that copies \a old file */
  File(const File &old) { Copy(old); }

  /** file assignment operator that copies \a old file */
  File &operator=(const File &old) {
    Copy(old);
    return *this;
  }

  /** copy \a old file */
  void Copy(const File &old) {
    filename_ = old.filename_;
    hermes_fd_ = old.hermes_fd_;
    hermes_fh_ = old.hermes_fh_;
    hermes_mpi_fh_ = old.hermes_mpi_fh_;
    status_ = old.status_;
  }

  /** file comparison operator */
  bool operator==(const File &other) const {
    return (hermes_fd_ == other.hermes_fd_) &&
        (hermes_fh_ == other.hermes_fh_) &&
        (hermes_mpi_fh_ == other.hermes_mpi_fh_);
  }

  /** return hash value of this class  */
  std::size_t hash() const {
    std::size_t result;
    std::size_t h1 = std::hash<int>{}(hermes_fd_);
    std::size_t h2 = std::hash<void*>{}(hermes_fh_);
    std::size_t h3 = std::hash<void*>{}(hermes_mpi_fh_);
    result = h1 ^ h2 ^ h3;
    return result;
  }
};

/** Any relevant statistics from the I/O client */
struct AdapterStat {
  std::string path_;     /**< The URL of this file */
  int flags_;            /**< open() flags for POSIX */
  bitfield32_t hflags_;  /**< Flags used by FS adapter */
  mode_t st_mode_;       /**< protection */
  uid_t st_uid_;         /**< user ID of owner */
  gid_t st_gid_;         /**< group ID of owner */
  size_t st_ptr_;        /**< current ptr of FILE */
  size_t file_size_;     /**< Size of file at backend at time of open */
  timespec st_atim_;     /**< time of last access */
  timespec st_mtim_;     /**< time of last modification */
  timespec st_ctim_;     /**< time of last status change */
  std::string mode_str_; /**< mode used for fopen() */
  AdapterMode adapter_mode_;  /**< Mode used for adapter */

  int fd_;          /**< real file descriptor */
  FILE *fh_;        /**< real STDIO file handler */
  MPI_File mpi_fh_; /**< real MPI file handler */

  int amode_;      /**< access mode (MPI) */
  MPI_Info info_;  /**< Info object (handle) */
  MPI_Comm comm_;  /**< Communicator for the file.*/
  bool atomicity_; /**< Consistency semantics for data-access */

  Bucket bkt_id_; /**< bucket associated with the file */
  /** Page size used for file */
  size_t page_size_;

  /** Default constructor */
  AdapterStat()
      : flags_(0),
        hflags_(),
        st_mode_(),
        st_ptr_(0),
        file_size_(0),
        st_atim_(),
        st_mtim_(),
        st_ctim_(),
        adapter_mode_(AdapterMode::kNone),
        fd_(-1),
        fh_(nullptr),
        mpi_fh_(nullptr),
        amode_(0),
        comm_(MPI_COMM_SELF),
        atomicity_(false) {}

  /** Update to the current time */
  void UpdateTime() {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    st_mtim_ = ts;
    st_ctim_ = ts;
  }

  /** compare \a a BLOB and \a b BLOB.*/
  static bool CompareBlobs(const std::string &a, const std::string &b) {
    return std::stol(a) < std::stol(b);
  }
};

}  // namespace hermes::adapter

#endif  // LABSTOR_TASKS_HERMES_ADAPTERS_INCLUDE_HERMES_ADAPTERS_FILESYSTEM_STRUCTS_H_
