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
#include "generator/primitives/bug_types/spatial/flow/flow.h"
#include "generator/primitives/bug_types/spatial/origin_target_relation/origin_target_relation.h"
#include "generator/primitives/regions/region.h"


class SpatialBugType: public Property
{
public:
  explicit SpatialBugType(std::string name): Property(name) {}
  virtual ~SpatialBugType() = default;

  virtual bool accepts(std::shared_ptr<Flow> flow) const = 0;
  virtual bool accepts(std::shared_ptr<OriginTargetRelation> origin_target_relation) const = 0;
  virtual bool accepts(std::shared_ptr<AccessLocation> access_location) const = 0;

  virtual std::vector< std::shared_ptr<OriginTargetCodeCanvas> > generate(
    std::shared_ptr<Region> origin,
    std::shared_ptr<Region> target,
    std::shared_ptr<OriginTargetRelation> origin_target_relation,
    std::shared_ptr<Flow> flow,
    std::shared_ptr<AccessAction> access_action,
    std::shared_ptr<AccessLocation> access_location
    ) const = 0;

  virtual std::vector< std::shared_ptr<OriginTargetCodeCanvas> > generate_validation(
    std::shared_ptr<Region> origin,
    std::shared_ptr<Region> target,
    std::shared_ptr<OriginTargetRelation> origin_target_relation,
    std::shared_ptr<Flow> flow,
    std::shared_ptr<AccessAction> access_action,
    std::shared_ptr<AccessLocation> access_location
    ) const = 0;
};
