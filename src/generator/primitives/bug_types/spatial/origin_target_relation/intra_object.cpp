/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "intra_object.h"

#include "misc.h"
#include "generator/primitives/regions/global_region.h"
#include "generator/primitives/regions/heap_region.h"
#include "generator/primitives/regions/stack_region.h"


bool IntraObject::accepts(std::shared_ptr<Region> origin, std::shared_ptr<Region> target) const
{
  if ( is_a<StackRegion>(origin) && is_a<StackRegion>(target) ) return true;
  if ( is_a<HeapRegion>(origin) && is_a<HeapRegion>(target) ) return true;
  if ( is_a<GlobalRegion>(origin) && is_a<GlobalRegion>(target) ) return true;
  return false;
}


std::vector< std::shared_ptr<OriginTargetCodeCanvas> > IntraObject::generate(
  CodeCanvas &canvas,
  std::shared_ptr<Region> origin,
  size_t origin_size,
  std::shared_ptr<Region> target,
  size_t target_size
) const
{
  std::vector< std::shared_ptr<OriginTargetCodeCanvas> > variants;
  std::shared_ptr<CodeCanvas> canvas_ptr = std::make_shared<CodeCanvas>(canvas);

  // variant 1: target declared after origin
  std::shared_ptr<RegionCodeCanvas> region_canvas = origin->generate(
    canvas_ptr, "s", "origin", origin_size, "target", target_size, true
  );
  ssize_t distance_value = static_cast<ssize_t>(origin_size);
  region_canvas->add_to_f_body(
    "%distance = arith.constant " + std::to_string(distance_value) + " : index"
  );
  region_canvas->add_to_f_body(
    "%distance_negated = arith.subi %c0, %distance : index"
  );

  auto variant = std::make_shared<OriginTargetCodeCanvas>(
    region_canvas, target_size, origin_size, "s_target", "s_origin",
    "distance", "distance_negated", true, false, distance_value
  );
  variant->set_lifetime_pos(region_canvas->get_lifetime_pos());
  variant->add_variant_description_line("target declared after origin");
  variants.push_back(variant);

  // variant 2: target declared before origin
  region_canvas = origin->generate(
    canvas_ptr, "s", "target", target_size, "origin", origin_size, true
  );
  distance_value = -static_cast<ssize_t>(target_size);
  region_canvas->add_to_f_body(
    "%distance = arith.constant " + std::to_string(std::abs(distance_value)) + " : index"
  );
  region_canvas->add_to_f_body(
    "%distance_negated = arith.subi %c0, %distance : index"
  );

  variant = std::make_shared<OriginTargetCodeCanvas>(
    region_canvas, target_size, origin_size, "s_target", "s_origin",
    "distance", "distance_negated", true, false, distance_value
  );
  variant->set_lifetime_pos(region_canvas->get_lifetime_pos());
  variant->add_variant_description_line("target declared before origin");
  variants.push_back(variant);

  return variants;
}
