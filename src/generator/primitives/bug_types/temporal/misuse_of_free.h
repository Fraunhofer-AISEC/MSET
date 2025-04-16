/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include "temporal_bug_type.h"

class MisuseOfFree: public TemporalBugType
{
public:
  MisuseOfFree();

  bool accepts(std::shared_ptr<MemoryState> memory_state) override;
  bool accepts(std::shared_ptr<Region> region) override;
  bool accepts(std::shared_ptr<AccessLocation> access_location) override;

  std::vector< std::shared_ptr<RegionCodeCanvas> > generate(
    std::shared_ptr<MemoryState> memory_state,
    std::shared_ptr<Region> memory_region,
    std::shared_ptr<AccessAction> access_action,
    std::shared_ptr<AccessLocation> access_location
    ) const override;

  std::vector< std::shared_ptr<RegionCodeCanvas> > generate_validation(
    std::shared_ptr<MemoryState> memory_state,
    std::shared_ptr<Region> memory_region,
    std::shared_ptr<AccessAction> access_action,
    std::shared_ptr<AccessLocation> access_location
    ) const override;
};
