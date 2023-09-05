//
// Created by llogan on 7/1/23.
//

#include "basic_test.h"
#include "labstor/api/labstor_client.h"
#include "labstor_admin/labstor_admin.h"
#include "hermes_mdm/hermes_mdm.h"
#include "hermes/bucket.h"
#include <mpi.h>

TEST_CASE("TestHermesPut") {
  int rank, nprocs;
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  if (rank == 0) {
    // Initialize Hermes on all nodes
    HERMES->ClientInit();

    // Create a bucket
    HILOG(kInfo, "WE ARE HERE!!!")
    hermes::Context ctx;
    hermes::Bucket bkt("hello");
    HILOG(kInfo, "BUCKET LOADED!!!")

    size_t count = 16;
    size_t off = rank * count;
    int max_blobs = 16;

    for (size_t i = 1; i < 2; ++i) {
      HILOG(kInfo, "Iteration: {}", i);
      // Put a blob
      hermes::Blob blob(KILOBYTES(4));
      memset(blob.data(), i, blob.size());
      // hermes::BlobId blob_id(hermes::BlobId::GetNull());
      hermes::BlobId blob_id(2, 0);
      bkt.Put(std::to_string(i % max_blobs), blob, blob_id, ctx);

      // Get a blob
      HILOG(kInfo, "Put {} returned successfully", i);
      // size_t size = bkt.GetBlobSize(blob_id);
      // REQUIRE(blob.size() == size);
    }
  }
  sleep(10);
  MPI_Barrier(MPI_COMM_WORLD);
}

TEST_CASE("TestHermesPutGet") {
  int rank, nprocs;
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  // Initialize Hermes on all nodes
  HERMES->ClientInit();

  // Create a bucket
  HILOG(kInfo, "WE ARE HERE!!!")
  hermes::Context ctx;
  hermes::Bucket bkt("hello");
  HILOG(kInfo, "BUCKET LOADED!!!")

  size_t count = 16;
  size_t off = rank * count;
  int max_blobs = 16;
  for (size_t i = off; i < count; ++i) {
    HILOG(kInfo, "Iteration: {}", i);
    // Put a blob
    hermes::Blob blob(KILOBYTES(4));
    memset(blob.data(), i, blob.size());
    hermes::BlobId blob_id(hermes::BlobId::GetNull());
    bkt.Put(std::to_string(i % max_blobs), blob, blob_id, ctx);

    // Get a blob
     hermes::Blob blob2;
     bkt.Get(blob_id, blob2, ctx);
     REQUIRE(blob == blob2);
  }
}