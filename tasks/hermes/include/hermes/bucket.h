//
// Created by lukemartinlogan on 7/9/23.
//

#ifndef LABSTOR_TASKS_HERMES_INCLUDE_HERMES_BUCKET_H_
#define LABSTOR_TASKS_HERMES_INCLUDE_HERMES_BUCKET_H_

#include "hermes_types.h"
#include "hermes_mdm/hermes_mdm.h"
#include "hermes_dpe/hermes_dpe.h"
#include "hermes/config_manager.h"

namespace hermes {

class Bucket {
 private:
  mdm::Client *mdm_;
  dpe::Client *dpe_;
  TagId id_;
  std::string name_;
  Context ctx_;
  bool did_create_;

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
                  Context &ctx,
                  size_t backend_size = 0) {
    mdm_ = &HERMES_CONF->mdm_;
    dpe_ = &HERMES_CONF->dpe_;
  }

  /**
   * Get an existing bucket.
   * */
  explicit Bucket(TagId tag_id) {
    id_ = tag_id;
    did_create_ = false;
    mdm_ = &HERMES_CONF->mdm_;
    dpe_ = &HERMES_CONF->dpe_;
  }

  /**
   * Check if the bucket was created in the constructor
   * */
  bool DidCreate() {
    return did_create_;
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
  }

  /**
   * Get the current size of the bucket
   * */
  size_t GetSize() {
  }

  /**
   * Update the size of the bucket
   * Needed for the adapters for now.
   * */
  void SetSize(size_t new_size) {
  }

  /**
   * Rename this bucket
   * */
  void Rename(const std::string &new_bkt_name) {
  }

  /**
   * Clears the buckets contents, but doesn't destroy its metadata
   * */
  void Clear() {
  }

  /**
   * Destroys this bucket along with all its contents.
   * */
  void Destroy() {
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
  }

  /**
   * Get the name of a blob from the blob id
   *
   * @param blob_id the blob_id
   * @param blob_name the name of the blob
   * @return The Status of the operation
   * */
  std::string GetBlobName(const BlobId &blob_id) {
  }


  /**
   * Get the score of a blob from the blob id
   *
   * @param blob_id the blob_id
   * @return The Status of the operation
   * */
  float GetBlobScore(const BlobId &blob_id) {
  }

  /**
   * Label \a blob_id blob with \a tag_name TAG
   * */
  Status TagBlob(BlobId &blob_id,
                 TagId &tag_id) {
  }

  /**
   * Put \a blob_name Blob into the bucket
   * */
  Status Put(const std::string &blob_name,
             const Blob &blob,
             BlobId &blob_id,
             Context &ctx) {
    dpe_->Put(blob_name, )
  }

  /**
   * Get \a blob_id Blob from the bucket
   * */
  Status Get(BlobId blob_id,
             Blob &blob,
             Context &ctx) {
    // Send message to MDM to get blob
  }

  /**
   * Determine if the bucket contains \a blob_id BLOB
   * */
  bool ContainsBlob(const std::string &blob_name,
                    BlobId &blob_id) {
  }

  /**
   * Determine if the bucket contains \a blob_id BLOB
   * */
  bool ContainsBlob(BlobId blob_id) {
  }

  /**
   * Rename \a blob_id blob to \a new_blob_name new name
   * */
  void RenameBlob(BlobId blob_id, std::string new_blob_name, Context &ctx) {
  }

  /**
   * Delete \a blob_id blob
   * */
  void DestroyBlob(BlobId blob_id, Context &ctx) {
  }

  /**
   * Get the set of blob IDs contained in the bucket
   * */
  std::vector<BlobId> GetContainedBlobIds() {
  }
};

}  // namespace hermes

#endif  // LABSTOR_TASKS_HERMES_INCLUDE_HERMES_BUCKET_H_
