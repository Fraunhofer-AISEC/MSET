/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "inter_object.h"
#include <random>

#include "misc.h"

#ifndef MAX_DISTANCE
#define MAX_DISTANCE 1024
#endif

bool InterObject::accepts(std::shared_ptr<Region> origin, std::shared_ptr<Region> target) const
{
  // accept any combinations
  return true;
}

static int generate_rand_distance()
{
  static std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution<> dist(0, MAX_DISTANCE);
  return dist(gen);
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
  int rand_distance = generate_rand_distance();

  // variant 1: origin first, target second (distance positive)
  std::shared_ptr<RegionCodeCanvas> region_canvas = origin->generate(
    canvas_ptr, "parent", "origin", origin_size, "target", target_size, true, rand_distance
  );
  ssize_t distance_value = static_cast<ssize_t>(origin_size) + rand_distance;
  region_canvas->add_to_f_body(
    "%distance = arith.constant " + std::to_string(distance_value) + " : index"
  );
  region_canvas->add_to_f_body(
    "%distance_negated = arith.subi %c0, %distance : index"
  );

  auto variant = std::make_shared<OriginTargetCodeCanvas>(
    region_canvas, target_size, origin_size, "parent_target", "parent_origin",
    "distance", "distance_negated", true, false, distance_value
  );
  variant->set_lifetime_pos(region_canvas->get_lifetime_pos());
  variant->add_variant_description_line("target declared after origin");
  variants.push_back(variant);

  if ( are_the_same_type(origin, target) )
  {
    // variant 2: target first, origin second (distance negative)
    region_canvas = origin->generate(
      canvas_ptr, "parent", "target", target_size, "origin", origin_size, true, rand_distance
    );
    distance_value = -(static_cast<ssize_t>(target_size) + rand_distance);
    region_canvas->add_to_f_body(
      "%distance = arith.constant " + std::to_string(std::abs(distance_value)) + " : index"
    );
    region_canvas->add_to_f_body(
      "%distance_negated = arith.subi %c0, %distance : index"
    );

    variant = std::make_shared<OriginTargetCodeCanvas>(
      region_canvas, target_size, origin_size, "parent_target", "parent_origin",
      "distance", "distance_negated", true, true, distance_value
    );
    variant->set_lifetime_pos(region_canvas->get_lifetime_pos());
    variant->add_variant_description_line("target declared before origin");
    variants.push_back(variant);
  }

  return variants;
}
