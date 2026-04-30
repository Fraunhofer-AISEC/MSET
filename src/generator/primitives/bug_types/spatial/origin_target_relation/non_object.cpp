/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "non_object.h"

#include "misc.h"
#include "generator/primitives/regions/global_region.h"
#include "generator/primitives/regions/heap_region.h"
#include "generator/primitives/regions/stack_region.h"


bool NonObject::accepts(std::shared_ptr<Region> origin, std::shared_ptr<Region> target) const
{
  // accept only same region origin and targets
  if ( is_a<StackRegion>(origin) && is_a<StackRegion>(target) ) return true;
  if ( is_a<HeapRegion>(origin) && is_a<HeapRegion>(target) ) return true;
  if ( is_a<GlobalRegion>(origin) && is_a<GlobalRegion>(target) ) return true;
  return false;
}


std::vector< std::shared_ptr<OriginTargetCodeCanvas> > NonObject::generate(
  CodeCanvas &canvas,
  std::shared_ptr<Region> origin,
  size_t origin_size,
  std::shared_ptr<Region> target,
  size_t target_size
) const
{
  std::vector< std::shared_ptr<OriginTargetCodeCanvas> > variants;
  std::shared_ptr<CodeCanvas> canvas_ptr = std::make_shared<CodeCanvas>(canvas);
  std::shared_ptr<RegionCodeCanvas> origin_canvas = origin->generate(canvas_ptr, "origin", origin_size, true);

  // overflow variant: target after origin
  ssize_t distance_up = static_cast<ssize_t>(origin_size);
  origin_canvas->add_to_f_body(
    "%distance = arith.constant " + std::to_string(distance_up) + " : index"
  );
  origin_canvas->add_to_f_body(
    "%distance_negated = arith.subi %c0, %distance : index"
  );

  auto variant = std::make_shared<OriginTargetCodeCanvas>(
    origin_canvas, 1, 8, "origin", "origin",
    "distance", "distance_negated", /*is_target_allocated=*/false, false, distance_up
  );
  variant->set_lifetime_pos(origin_canvas->get_lifetime_pos());
  variant->add_variant_description_line("target after origin (overflow)");
  variants.push_back(variant);

  // underflow variant: target before origin
  ssize_t distance_down = static_cast<ssize_t>(origin_size);
  origin_canvas->add_to_f_body(
    "%underflow_dist = arith.constant " + std::to_string(distance_down) + " : index"
  );

  variant = std::make_shared<OriginTargetCodeCanvas>(
    origin_canvas, 1, 8, "origin", "origin",
    "underflow_dist", "N/A", /*is_target_allocated=*/false, /*requires_underflow=*/true, -1
  );
  variant->set_lifetime_pos(origin_canvas->get_lifetime_pos());
  variant->add_variant_description_line("target before origin (underflow)");
  variants.push_back(variant);

  return variants;
}
