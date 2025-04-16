/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include "generator/primitives/bug_types/spatial/origin_target_relation/origin_target_relation.h"


class NonObject: public OriginTargetRelation
{
public:
  NonObject():
    OriginTargetRelation("non_object")
  {
  }

  bool accepts(std::shared_ptr<Region> origin, std::shared_ptr<Region> target) const override;
  std::vector< std::shared_ptr<OriginTargetCodeCanvas> > generate(
    CodeCanvas &canvas,
    std::shared_ptr<Region> origin,
    size_t origin_size,
    std::shared_ptr<Region> target,
    size_t target_size
  ) const override;
};
