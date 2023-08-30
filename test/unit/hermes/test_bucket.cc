//
// Created by llogan on 7/1/23.
//

#include "basic_test.h"
#include "labstor/api/labstor_client.h"
#include "labstor_admin/labstor_admin.h"
#include "hermes_mdm/hermes_mdm.h"
#include "hermes/bucket.h"


TEST_CASE("TestHermesBucket") {
  // Initialize Hermes on all nodes
  HERMES->ClientInit();

  // Create a bucket
  hermes::Context ctx;
  hermes::Bucket bkt("hello", ctx);

  for (size_t i = 0; i < 1024; ++i) {
    HILOG(kInfo, "Iteration: {}", i);
    // Put a blob
    hermes::Blob blob(MEGABYTES(1));
    memset(blob.data(), 10, blob.size());
    hermes::BlobId blob_id;
    bkt.Put("hello", blob, blob_id, ctx);

    // Get a blob
    hermes::Blob blob2;
    bkt.Get(blob_id, blob2, ctx);
    REQUIRE(blob == blob2);
  }
}