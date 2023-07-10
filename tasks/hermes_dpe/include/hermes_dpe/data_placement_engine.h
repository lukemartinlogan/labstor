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

#ifndef HERMES_DATA_PLACEMENT_ENGINE_H_
#define HERMES_DATA_PLACEMENT_ENGINE_H_

#include <map>

#include "hermes/hermes.h"
#include "hermes/hermes_types.h"
#include "hermes_mdm/hermes_mdm.h"

namespace hermes {

/**
 A class to represent data placement engine
*/
class DPE {
 public:
  /** Constructor. */
  DPE();

  /** Destructor. */
  virtual ~DPE() = default;

  /**
   * Calculate the placement of a set of blobs using a particular
   * algorithm given a context.
   * */
  virtual Status Placement(const std::vector<size_t> &blob_sizes,
                           std::vector<TargetInfo> &targets,
                           api::Context &ctx,
                           std::vector<PlacementSchema> &output) = 0;

  /**
   * Calculate the total placement for all blobs and output a schema for
   * each blob.
   * */
  Status CalculatePlacement(const std::vector<size_t> &blob_sizes,
                            std::vector<PlacementSchema> &output,
                            api::Context &api_context);
};

}  // namespace hermes
#endif  // HERMES_DATA_PLACEMENT_ENGINE_H_
