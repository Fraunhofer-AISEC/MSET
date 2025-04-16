/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include <memory>
#include <string>

#include "generator/code_canvas.h"
#include "generator/primitives/access_types/access_action.h"
#include "generator/primitives/access_types/access_location.h"
#include "memory_state/memory_state.h"


class TemporalBugType: public Property
{
public:
  explicit TemporalBugType(std::string name);
  virtual ~TemporalBugType() = default;

  virtual bool accepts(std::shared_ptr<MemoryState> memory_state) = 0;
  virtual bool accepts(std::shared_ptr<Region> region) = 0;
  virtual bool accepts(std::shared_ptr<AccessLocation> access_location) = 0;

  virtual std::vector< std::shared_ptr<RegionCodeCanvas> > generate(
    std::shared_ptr<MemoryState> memory_state,
    std::shared_ptr<Region> memory_region,
    std::shared_ptr<AccessAction> access_action,
    std::shared_ptr<AccessLocation> access_location
    ) const = 0;

  virtual std::vector< std::shared_ptr<RegionCodeCanvas> > generate_validation(
    std::shared_ptr<MemoryState> memory_state,
    std::shared_ptr<Region> memory_region,
    std::shared_ptr<AccessAction> access_action,
    std::shared_ptr<AccessLocation> access_location
    ) const = 0;
};
