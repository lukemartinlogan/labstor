//
// Created by lukemartinlogan on 7/9/23.
//

#ifndef LABSTOR_TASKS_HERMES_INCLUDE_HERMES_BUCKET_H_
#define LABSTOR_TASKS_HERMES_INCLUDE_HERMES_BUCKET_H_

#include "hermes_types.h"
#include "hermes_mdm/hermes_mdm.h"
#include "hermes/config_manager.h"

namespace hermes {

#define HERMES_BUCKET_IS_FILE (1 << 1)

class Bucket {
 public:
  mdm::Client *mdm_;
  blob_mdm::Client *blob_mdm_;
  bucket_mdm::Client *bkt_mdm_;
  TagId id_;
  std::string name_;
  Context ctx_;
  bitfield32_t flags_;

 public:
  /**====================================
   * Bucket Operations
   * ===================================*/

  /**
   * Get or create \a bkt_name bucket.
   *
   * Called from hermes.h in GetBucket(). Should not
   * be used directly.
   * */
  explicit Bucket(const std::string &bkt_name,
                  size_t backend_size = 0) {
    mdm_ = &HERMES->mdm_;
    blob_mdm_ = &HERMES->blob_mdm_;
    bkt_mdm_ = &HERMES->bkt_mdm_;
    HILOG(kInfo, "GETTING BUCKET: {}", bkt_name);
    bkt_mdm_->GetOrCreateTagRoot(hshm::charbuf(bkt_name), true,
                                 std::vector<TraitId>(), backend_size);
  }

  /**
   * Get or create \a bkt_name bucket.
   *
   * Called from hermes.h in GetBucket(). Should not
   * be used directly.
   * */
  explicit Bucket(const std::string &bkt_name,
                  Context &ctx,
                  size_t backend_size = 0) {
    mdm_ = &HERMES->mdm_;
    blob_mdm_ = &HERMES->blob_mdm_;
    bkt_mdm_ = &HERMES->bkt_mdm_;
    bkt_mdm_->GetOrCreateTagRoot(hshm::charbuf(bkt_name), true,
                                 std::vector<TraitId>(), backend_size);
  }

  /**
   * Get an existing bucket.
   * */
  explicit Bucket(TagId tag_id) {
    id_ = tag_id;
    mdm_ = &HERMES->mdm_;
    blob_mdm_ = &HERMES->blob_mdm_;
    bkt_mdm_ = &HERMES->bkt_mdm_;
  }

  /** Default constructor */
  Bucket() = default;

  /** Default copy constructor */
  Bucket(const Bucket &other) = default;

  /** Default copy assign */
  Bucket& operator=(const Bucket &other) = default;

  /** Default move constructor */
  Bucket(Bucket &&other) = default;

  /** Default move assign */
  Bucket& operator=(Bucket &&other) = default;

 public:
  /**
   * Get the name of this bucket. Name is cached instead of
   * making an RPC. Not coherent if Rename is called.
   * */
  const std::string& GetName() const {
    return name_;
  }

  /**
   * Get the identifier of this bucket
   * */
  TagId GetId() const {
    return id_;
  }

  /**
   * Get the context object of this bucket
   * */
  Context& GetContext() {
    return ctx_;
  }

  /**
   * Attach a trait to the bucket
   * */
  void AttachTrait(TraitId trait_id) {
    // TODO(llogan)
  }

  /**
   * Get the current size of the bucket
   * */
  size_t GetSize() {
    // TODO(llogan)
    return 0;
  }

  /**
   * Rename this bucket
   * */
  void Rename(const std::string &new_bkt_name) {
    bkt_mdm_->RenameTagRoot(id_, hshm::to_charbuf(new_bkt_name));
  }

  /**
   * Clears the buckets contents, but doesn't destroy its metadata
   * */
  void Clear() {
    bkt_mdm_->TagClearBlobsRoot(id_);
  }

  /**
   * Destroys this bucket along with all its contents.
   * */
  void Destroy() {
    bkt_mdm_->DestroyTagRoot(id_);
  }

  /**
   * Check if this bucket is valid
   * */
  bool IsNull() {
    return id_.IsNull();
  }

 public:
  /**====================================
   * Blob Operations
   * ===================================*/

  /**
   * Get the id of a blob from the blob name
   *
   * @param blob_name the name of the blob
   * @param blob_id (output) the returned blob_id
   * @return The Status of the operation
   * */
  Status GetBlobId(const std::string &blob_name, BlobId &blob_id) {
    blob_id = blob_mdm_->GetBlobIdRoot(id_, hshm::to_charbuf(blob_name));
    return Status();
  }

  /**
   * Get the name of a blob from the blob id
   *
   * @param blob_id the blob_id
   * @param blob_name the name of the blob
   * @return The Status of the operation
   * */
  std::string GetBlobName(const BlobId &blob_id) {
    return blob_mdm_->GetBlobNameRoot(blob_id);
  }


  /**
   * Get the score of a blob from the blob id
   *
   * @param blob_id the blob_id
   * @return The Status of the operation
   * */
  float GetBlobScore(const BlobId &blob_id) {
    return blob_mdm_->GetBlobScoreRoot(blob_id);
  }

  /**
   * Label \a blob_id blob with \a tag_name TAG
   * */
  Status TagBlob(BlobId &blob_id,
                 TagId &tag_id) {
    bkt_mdm_->TagAddBlobRoot(tag_id, blob_id);
    return Status();
  }

  /**
   * Put \a blob_name Blob into the bucket
   * */
  template<bool PARTIAL, bool APPEND>
  Status Put(const std::string &blob_name,
             const Blob &blob,
             BlobId &blob_id,
             Context &ctx) {
    // Copy data to shared memory
    hipc::Pointer p = LABSTOR_CLIENT->AllocateBuffer(blob.size());
    char *data = LABSTOR_CLIENT->GetPrivatePointer(p);
    memcpy(data, blob.data(), blob.size());
    // Put to shared memory
    hshm::charbuf blob_name_buf = hshm::to_charbuf(blob_name);
    if (blob_id.IsNull()) {
      blob_id = blob_mdm_->GetOrCreateBlobIdRoot(id_, blob_name_buf);
    }
    if constexpr(!PARTIAL) {
      blob_mdm_->PutBlobRoot(id_, blob_name_buf,
                             blob_id, 0, blob.size(), p, ctx.blob_score_,
                             bitfield32_t(HERMES_BLOB_REPLACE));
    } else {
      blob_mdm_->PutBlobRoot(id_, blob_name_buf,
                             blob_id, 0, blob.size(), p, ctx.blob_score_,
                             bitfield32_t(0));
    }
    return Status();
  }

  /**
   * Put \a blob_name Blob into the bucket
   * */
  Status Put(const std::string &blob_name,
             const Blob &blob,
             BlobId &blob_id,
             Context &ctx) {
    return Put<false, false>(blob_name, blob, blob_id, ctx);
  }

  /**
   * Put \a blob_name Blob into the bucket
   * */
  Status PartialPut(const std::string &blob_name,
                    const Blob &blob,
                    size_t blob_off,
                    BlobId &blob_id,
                    Context &ctx) {
    return Put<true, false>(blob_name, blob, blob_id, ctx);
  }

  /**
   * Append \a blob_name Blob into the bucket
   * */
  Status Append(const Blob &blob, Context &ctx) {
    return Status();
//    BlobId blob_id(BlobId::GetNull());
//    return Put<false, true>(blob_name, blob, blob_id, ctx);
  }


  /**
   * Get \a blob_id Blob from the bucket
   * */
  Status Get(BlobId blob_id,
             Blob &blob,
             Context &ctx) {
    // Get from shared memory
    ssize_t data_size = -1;
    hipc::Pointer data_p = hipc::Pointer::GetNull();
    blob_mdm_->GetBlobRoot(blob_id, 0, data_size, data_p);
    char *data = LABSTOR_CLIENT->GetPrivatePointer(data_p);
    // Copy data to blob
    // TODO(llogan): intercept mmap to avoid copy
    blob.resize(data_size);
    memcpy(blob.data(), data, data_size);
    LABSTOR_CLIENT->FreeBuffer(data_p);
    return Status();
  }

  /**
   * Put \a blob_name Blob into the bucket
   * */
  Status PartialGet(const std::string &blob_name,
                    Blob &blob,
                    size_t blob_off_,
                    ssize_t &data_size,
                    BlobId &blob_id,
                    Context &ctx) {
    // Get from shared memory
    hipc::Pointer data_p = LABSTOR_CLIENT->AllocateBuffer(data_size);
    blob_mdm_->GetBlobRoot(blob_id, blob_off_, data_size, data_p);
    char *data = LABSTOR_CLIENT->GetPrivatePointer(data_p);
    // TODO(llogan): intercept mmap to avoid copy
    blob.resize(data_size);
    memcpy(blob.data(), data, data_size);
    LABSTOR_CLIENT->FreeBuffer(data_p);
    return Status();
  }

  /**
   * Determine if the bucket contains \a blob_id BLOB
   * */
  bool ContainsBlob(const std::string &blob_name,
                    BlobId &blob_id) {
    blob_id = blob_mdm_->GetBlobIdRoot(id_, hshm::to_charbuf(blob_name));
    return !blob_id.IsNull();
  }

  /**
   * Rename \a blob_id blob to \a new_blob_name new name
   * */
  void RenameBlob(BlobId blob_id, std::string new_blob_name, Context &ctx) {
    blob_mdm_->RenameBlobRoot(blob_id, hshm::to_charbuf(new_blob_name));
  }

  /**
   * Delete \a blob_id blob
   * */
  void DestroyBlob(BlobId blob_id, Context &ctx) {
    // TODO(llogan): Make apart of bkt_mdm_ instead
    blob_mdm_->DestroyBlobRoot(blob_id);
  }

  /**
   * Get the set of blob IDs contained in the bucket
   * */
  std::vector<BlobId> GetContainedBlobIds() {
    // TODO(llogan)
    return {};
  }
};

}  // namespace hermes

#endif  // LABSTOR_TASKS_HERMES_INCLUDE_HERMES_BUCKET_H_
