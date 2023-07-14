//
// Created by llogan on 7/1/23.
//

#include "basic_test.h"
#include "labstor/api/labstor_client.h"
#include "labstor_admin/labstor_admin.h"
#include "hermes_mdm/hermes_mdm.h"
#include "hermes/bucket.h"


TEST_CASE("TestHermes") {
  // Initialize Hermes on all nodes
  HERMES->ClientInit();

  // Create a bucket
  hermes::Context ctx;
  hermes::Bucket bkt("hello", ctx);
}