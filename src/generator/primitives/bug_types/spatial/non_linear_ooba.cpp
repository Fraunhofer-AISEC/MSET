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
#include "generator/primitives/bug_types/spatial/origin_target_relation/non_object.h"

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
    return 42;
  */
  CodeCanvas variant;
  variant.add_global("func.func private @exit(%arg0: i32) -> ()");

  variant.add_test_case_description_line("Origin: " + origin->get_name());
  variant.add_test_case_description_line("Target: " + target->get_name());
  variant.add_test_case_description_line("Bug type: " + origin_target_relation->get_printable_name() + ", non-linear OOBA, " + flow->get_name());
  variant.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  auto generate_preconditions_check_distance = std::bind(&Flow::generate_preconditions_check_distance, flow.get(), std::placeholders::_1);

  std::vector< std::shared_ptr<OriginTargetCodeCanvas> > origin_target_canvases = origin_target_relation->generate(
    variant, origin, 8, target, 8);

  for ( auto &origin_target_canvas : origin_target_canvases )
  {
    if ( origin_target_canvas->get_forces_underflow() && !is_a<Underflow>(flow) ) continue; // the origin-target requires an underflow, but this is not an underflow -> skip
    std::string distance = origin_target_canvas->get_distance();

    if ( distance == "N/A" ) continue;
    auto origin_target_canvas_copy = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas);
    ssize_t static_dist = origin_target_canvas->get_distance_static_value();
    std::string origin_offset = static_dist > 0 ? "0" : std::to_string(std::abs(static_dist));
    bool needs_strided = !is_a<NonObject>(origin_target_relation);
    std::vector<std::string> access_target_code = access_location->generate_at_index(
      access_action,
      origin_target_canvas_copy->get_origin_name(),
      distance,
      origin_target_canvas_copy->get_target_size(),
      generate_preconditions_check_distance,
      needs_strided,
      origin_offset
    );
    origin_target_canvas_copy->add_during_lifetime(access_target_code);
    origin_target_canvas_copy->add_during_lifetime("func.call @exit(%test_success) : (i32) -> ()");
    full_variants.push_back( origin_target_canvas_copy );

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
    return 42;
  */
  CodeCanvas variant;
  variant.add_global("func.func private @exit(%arg0: i32) -> ()");

  variant.add_test_case_description_line("Origin: " + origin->get_name());
  variant.add_test_case_description_line("Target: " + target->get_name());
  variant.add_test_case_description_line("Bug type: " + origin_target_relation->get_printable_name() + ", non-linear OOBA, " + flow->get_name());
  variant.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());


  std::vector< std::shared_ptr<OriginTargetCodeCanvas> > origin_target_canvases = origin_target_relation->generate(
    variant, origin, 8, target, 8);

  for ( auto &origin_target_canvas : origin_target_canvases )
  {
    if ( origin_target_canvas->get_forces_underflow() && !is_a<Underflow>(flow) ) continue; // the origin-target requires an underflow, but the for is not an underflow -> skip
    std::string distance = { "c0" };

    if ( distance == "N/A" ) continue;
    auto origin_target_canvas_copy = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas);
    std::string var_name_to_access;
    std::string var_offset = "0";
    if ( origin_target_canvas_copy->is_target_allocated() )
    {
      var_name_to_access = origin_target_canvas_copy->get_target_name();
      ssize_t static_dist = origin_target_canvas->get_distance_static_value();
      if ( static_dist > 0 )
        var_offset = std::to_string(static_dist);
    }
    else
    {
      var_name_to_access = origin_target_canvas_copy->get_origin_name();
    }
    bool needs_strided = !is_a<NonObject>(origin_target_relation);
    std::vector<std::string> access_target_code = access_location->generate_at_index(
      access_action,
      var_name_to_access,
      distance,
      origin_target_canvas_copy->get_target_size(),
      nullptr,
      needs_strided,
      var_offset
    );
    origin_target_canvas_copy->add_during_lifetime(access_target_code);
    origin_target_canvas_copy->add_during_lifetime("func.call @exit(%test_success) : (i32) -> ()");
    full_variants.push_back( origin_target_canvas_copy );

  }

  return full_variants;
}
