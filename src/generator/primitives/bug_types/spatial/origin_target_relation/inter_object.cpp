/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "inter_object.h"

#include "misc.h"


bool InterObject::accepts(std::shared_ptr<Region> origin, std::shared_ptr<Region> target) const
{
  // accept any combinations
  return true;
}

std::vector< std::shared_ptr<OriginTargetCodeCanvas> > InterObject::generate(
  CodeCanvas &canvas,
  std::shared_ptr<Region> origin,
  size_t origin_size,
  std::shared_ptr<Region> target,
  size_t target_size
) const
{
  std::vector< std::shared_ptr<OriginTargetCodeCanvas> > variants;
  std::shared_ptr<CodeCanvas> canvas_ptr = std::make_shared<CodeCanvas>(canvas);
  std::shared_ptr<RegionCodeCanvas> origin_canvas;
  std::shared_ptr<RegionCodeCanvas> target_canvas;

  origin_canvas = origin->generate(canvas_ptr, "origin", origin_size, true);
  std::string distance = "(ssize_t)(GET_ADDR_BITS(target) - GET_ADDR_BITS(origin))";
  std::string distance_negated = "-(ssize_t)(GET_ADDR_BITS(origin) - GET_ADDR_BITS(target))";
  target_canvas = target->generate(origin_canvas->get_lifetime_pos(), origin_canvas, "target", target_size, true);

  std::shared_ptr<OriginTargetCodeCanvas> variant = std::make_shared<OriginTargetCodeCanvas>( target_canvas, target_size, origin_size, "target", "origin", distance, distance_negated );
  variant->set_lifetime_pos( target_canvas->get_lifetime_pos() );
  variant->add_to_variant_description("target declared after origin");
  variants.push_back(variant);

  if ( are_the_same_type(origin, target) )
  {
    target_canvas = target->generate(canvas_ptr, "target", target_size, true);
    origin_canvas = origin->generate(target_canvas->get_lifetime_pos(), target_canvas, "origin", origin_size, true);
    variant = std::make_shared<OriginTargetCodeCanvas>( origin_canvas, target_size, origin_size, "target", "origin", distance, distance_negated );
    variant->set_lifetime_pos( origin_canvas->get_lifetime_pos() );
  variant->add_to_variant_description("target declared before origin");
    variants.push_back(variant);
  }

  return variants;
}
