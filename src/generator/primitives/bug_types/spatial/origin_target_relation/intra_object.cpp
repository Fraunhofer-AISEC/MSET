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
  // accept only same region origin and targets
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
  std::shared_ptr<RegionCodeCanvas> region_canvas = origin->generate(canvas_ptr, "s", "origin", origin_size, "target", target_size, true);
  bool is_pointer = is_a<HeapRegion>(origin);
  std::string distance;
  std::string distance_negated;
  std::string origin_access;
  std::string target_access;
  if ( is_pointer )
  {
    origin_access = "s->origin";
    target_access = "s->target";
  }
  else
  {
    origin_access = "s.origin";
    target_access = "s.target";
  }
  distance = "(ssize_t)(GET_ADDR_BITS(" + target_access + ") - GET_ADDR_BITS(" + origin_access + "))";
  distance_negated = "-(ssize_t)(GET_ADDR_BITS(" + origin_access + ") - GET_ADDR_BITS(" + target_access + "))";

  std::shared_ptr<OriginTargetCodeCanvas> variant = std::make_shared<OriginTargetCodeCanvas>( region_canvas, 8, origin_size, target_access, origin_access, distance, distance_negated );
  variant->set_lifetime_pos( region_canvas->get_lifetime_pos() );
  variant->add_variant_description_line("target declared after origin");
  variants.push_back(variant);

  region_canvas = origin->generate(canvas_ptr, "s", "target", target_size, "origin", origin_size, true);
  variant = std::make_shared<OriginTargetCodeCanvas>( region_canvas, 8, origin_size, target_access, origin_access, distance, distance_negated );
  variant->set_lifetime_pos( region_canvas->get_lifetime_pos() );
  variant->add_variant_description_line("target declared before origin");
  variants.push_back(variant);

  return variants;
}

