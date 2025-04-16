/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include "generator/primitives/bug_types/spatial/spatial_bug_type.h"


class NonLinearOOBA: public SpatialBugType
{
public:
  NonLinearOOBA():
    SpatialBugType("non_linear_ooba")
  {
  }

  bool accepts(std::shared_ptr<Flow> flow) const override;
  bool accepts(std::shared_ptr<OriginTargetRelation> origin_target_relation) const override;
  bool accepts(std::shared_ptr<AccessLocation> access_location) const override;

  std::vector<std::shared_ptr<OriginTargetCodeCanvas>> generate(std::shared_ptr<Region> origin,
    std::shared_ptr<Region> target, std::shared_ptr<OriginTargetRelation> origin_target_relation,
    std::shared_ptr<Flow> flow, std::shared_ptr<AccessAction> access_action,
    std::shared_ptr<AccessLocation> access_location) const override;

  std::vector<std::shared_ptr<OriginTargetCodeCanvas>> generate_validation(std::shared_ptr<Region> origin,
    std::shared_ptr<Region> target, std::shared_ptr<OriginTargetRelation> origin_target_relation,
    std::shared_ptr<Flow> flow, std::shared_ptr<AccessAction> access_action,
    std::shared_ptr<AccessLocation> access_location) const override;
};
