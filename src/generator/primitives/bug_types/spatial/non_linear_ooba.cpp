/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "non_linear_ooba.h"

#include "misc.h"
#include "generator/primitives/bug_types/spatial/flow/underflow.h"
#include "generator/primitives/bug_types/spatial/origin_target_relation/inter_object.h"
#include "generator/primitives/bug_types/spatial/origin_target_relation/intra_object.h"

bool NonLinearOOBA::accepts(std::shared_ptr<Flow> flow) const
{
  return true;
}

bool NonLinearOOBA::accepts(std::shared_ptr<OriginTargetRelation> origin_target_relation) const
{
  return is_a<InterObject>(origin_target_relation)
    || is_a<IntraObject>(origin_target_relation);
}

bool NonLinearOOBA::accepts(std::shared_ptr<AccessLocation> access_location) const
{
  return true;
}

std::vector<std::shared_ptr<OriginTargetCodeCanvas>> NonLinearOOBA::generate(
  std::shared_ptr<Region> origin,
  std::shared_ptr<Region> target,
  std::shared_ptr<OriginTargetRelation> origin_target_relation,
  std::shared_ptr<Flow> flow,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location
) const
{
  std::vector< std::shared_ptr<OriginTargetCodeCanvas> >full_variants;
  /*
    <target, origin>
    <action>(origin[distance(target,origin)], target_size) // access the target
    _exit(TEST_CASE_SUCCESSFUL_VALUE);
  */
  CodeCanvas variant;


  auto generate_preconditions_check_distance = std::bind(&Flow::generate_preconditions_check_distance, flow.get(), std::placeholders::_1);

  std::vector< std::shared_ptr<OriginTargetCodeCanvas> > origin_target_canvases = origin_target_relation->generate(
    variant, origin, 8, target, 8);

  for ( auto &origin_target_canvas : origin_target_canvases )
  {
    if ( origin_target_canvas->get_forces_underflow() && !is_a<Underflow>(flow) ) continue; // the origin-target requires an underflow, but the for is not an underflow -> skip
    std::vector< std::string > distance_variants = { origin_target_canvas->get_distance() };
    for ( auto &distance_variant : distance_variants )
    {
      if ( distance_variant == "N/A" ) continue;
      auto origin_target_canvas_copy = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas);
      std::vector<std::string> access_target_code = access_location->generate_at_index(
        access_action,
        origin_target_canvas_copy->get_origin_name(),
        distance_variant,
        origin_target_canvas_copy->get_target_size(),
        generate_preconditions_check_distance
      );
      origin_target_canvas_copy->add_during_lifetime(access_target_code);
      origin_target_canvas_copy->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
      full_variants.push_back( origin_target_canvas_copy );
    }
  }

  return full_variants;
}


std::vector<std::shared_ptr<OriginTargetCodeCanvas>> NonLinearOOBA::generate_validation(
  std::shared_ptr<Region> origin,
  std::shared_ptr<Region> target,
  std::shared_ptr<OriginTargetRelation> origin_target_relation,
  std::shared_ptr<Flow> flow,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location
) const
{
  std::vector< std::shared_ptr<OriginTargetCodeCanvas> >full_variants;
  /*
    <target, origin>
    <action>(target, target_size) // access the target
    _exit(TEST_CASE_SUCCESSFUL_VALUE);
  */
  CodeCanvas variant;

  std::vector< std::shared_ptr<OriginTargetCodeCanvas> > origin_target_canvases = origin_target_relation->generate(
    variant, origin, 8, target, 8);

  for ( auto &origin_target_canvas : origin_target_canvases )
  {
    if ( origin_target_canvas->get_forces_underflow() && !is_a<Underflow>(flow) ) continue; // the origin-target requires an underflow, but the for is not an underflow -> skip
    std::vector< std::string > distance_variants = { "0" };
    for ( auto &distance_variant : distance_variants )
    {
      if ( distance_variant == "N/A" ) continue;
      auto origin_target_canvas_copy = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas);
      std::vector<std::string> access_target_code = access_location->generate_at_index(
        access_action,
        origin_target_canvas_copy->get_target_name(),
        distance_variant,
        origin_target_canvas_copy->get_target_size(),
        nullptr
      );
      origin_target_canvas_copy->add_during_lifetime(access_target_code);
      origin_target_canvas_copy->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
      full_variants.push_back( origin_target_canvas_copy );
    }
  }

  return full_variants;
}
