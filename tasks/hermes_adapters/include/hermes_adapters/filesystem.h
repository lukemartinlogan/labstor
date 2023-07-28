/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Distributed under BSD 3-Clause license.                                   *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Illinois Institute of Technology.                        *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of Hermes. The full Hermes copyright notice, including  *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the top directory. If you do not  *
 * have access to the file, you may request a copy from help@hdfgroup.org.   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef HERMES_ADAPTER_FILESYSTEM_FILESYSTEM_H_
#define HERMES_ADAPTER_FILESYSTEM_FILESYSTEM_H_

#ifndef O_TMPFILE
#define O_TMPFILE 0x0
#endif

#include "hermes_adapters/posix/posix_api.h"
#include "hermes_adapters/mapper/mapper_factory.h"
#include "hermes/hermes.h"

namespace hermes::adapter {

/** The maximum length of a posix path */
static inline const int kMaxPathLen = 4096;

/** A class to represent file system */
class Filesystem {
 public:
  /** open \a f File in \a path*/
  hermes::Bucket Open(const std::string &path) {
    Context ctx;
    std::string abs_path = stdfs::absolute(path).string();
    size_t backend_size = GetBackendSize(path);
    hermes::Bucket bkt(abs_path, ctx, backend_size);
    bkt.flags_.SetBits(HERMES_BUCKET_IS_FILE);
    return bkt;
  }

  /** write */
  size_t Write(hermes::Bucket &bkt, const void *ptr,
               size_t off, size_t total_size,
               bool append, Context &ctx) {
    // Fragment I/O request into pages
    BlobPlacements mapping;
    auto mapper = MapperFactory::Get(MapperType::kBalancedMapper);
    mapper->map(off, total_size, ctx.page_size_, mapping);
    size_t data_offset = 0;

    // Perform a PartialPut for each page
    for (const auto &p : mapping) {
      const Blob page((const char*)ptr + data_offset, p.blob_size_);
      if (!append) {
        BlobId blob_id;
        std::string blob_name(p.CreateBlobName().str());
        bkt.PartialPut(blob_name, page, p.blob_off_, blob_id, ctx);
      } else {
        bkt.Append(page, ctx);
      }
      data_offset += p.blob_size_;
    }

    return data_offset;
  }

  /** read */
  size_t Read(hermes::Bucket &bkt, void *ptr,
              size_t off, size_t total_size,
              bool append, Context &ctx) {
    if (append) {
      return 0;
    }

    // Fragment I/O request into pages
    BlobPlacements mapping;
    auto mapper = MapperFactory::Get(MapperType::kBalancedMapper);
    mapper->map(off, total_size, ctx.page_size_, mapping);
    size_t data_offset = 0;

    // Perform a PartialPut for each page
    for (const auto &p : mapping) {
      Blob page((const char*)ptr + data_offset, p.blob_size_);
      ssize_t data_size;
      BlobId blob_id;
      std::string blob_name(p.CreateBlobName().str());
      bkt.PartialGet(blob_name, page, p.blob_off_, data_size, blob_id, ctx);
      data_offset += p.blob_size_;
    }

    return data_offset;
  }

  /** file size */
  size_t GetSize(hermes::Bucket &bkt) {
    return bkt.GetSize();
  }

  /** sync */
  void Sync(hermes::Bucket &bkt) {
    // Do nothing
  }

  /** truncate */
  void Truncate(hermes::Bucket &bkt, size_t new_size) {
  }

  /** close */
  int Close(hermes::Bucket &bkt) {
    // Do nothing
  }

  /** remove */
  int Remove(const std::string &pathname) {
    hermes::Context ctx;
    hermes::Bucket bkt(pathname, ctx);
    bkt.Destroy();
  }

 private:
  /** Get the size of a file on the backend filesystem */
  size_t GetBackendSize(const std::string &path) {
    size_t true_size = 0;
    int fd = HERMES_POSIX_API->open(path.c_str(), O_RDONLY);
    if (fd < 0) { return 0; }
    struct stat buf;
    HERMES_POSIX_API->fstat(fd, &buf);
    true_size = buf.st_size;
    HERMES_POSIX_API->close(fd);
  }

 public:
  /** Whether or not \a path PATH is tracked by Hermes */
  static bool IsPathTracked(const std::string &path) {
    /* if (!HERMES->IsInitialized()) {
      return false;
    } */
    std::string abs_path = stdfs::absolute(path).string();
    auto &paths = HERMES->client_config_.path_list_;
    // Check if path is included or excluded
    for (config::UserPathInfo &pth : paths) {
      if (abs_path.rfind(pth.path_) != std::string::npos) {
        if (abs_path == pth.path_ && pth.is_directory_) {
          // Do not include if path is a tracked directory
          return false;
        }
        return pth.include_;
      }
    }
    // Assume it is excluded
    return false;
  }
};

}  // namespace hermes::adapter

#define HERMES_FILESYSTEM_API \
  hshm::EasySingleton<hermes::adapter::Filesystem>::GetInstance()

#endif  // HERMES_ADAPTER_FILESYSTEM_FILESYSTEM_H_
