/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "linear_ooba.h"

#include <cassert>

#include "misc.h"
#include "generator/primitives/access_types/read_action.h"
#include "generator/primitives/bug_types/spatial/flow/underflow.h"
#include "generator/primitives/bug_types/spatial/flow/overflow.h"
#include "generator/primitives/bug_types/spatial/origin_target_relation/intra_object.h"
#include "generator/primitives/bug_types/spatial/origin_target_relation/non_object.h"

bool LinearOOBA::accepts(std::shared_ptr<Flow> flow) const
{
  return true;
}

bool LinearOOBA::accepts(std::shared_ptr<OriginTargetRelation> origin_target_relation) const
{
  return true;
}

bool LinearOOBA::accepts(std::shared_ptr<AccessLocation> access_location) const
{
  return true;
}

std::vector<std::shared_ptr<OriginTargetCodeCanvas>> LinearOOBA::generate(
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
    <target, origin, aux_ptr allocations> // aux_ptr points to origin
    <action>(aux_ptr, target) // aux_ptr reaches the target
    <action>(aux_ptr, target_size) // access the target
    return 42;
  */
  CodeCanvas variant;
  variant.add_global("func.func private @exit(%arg0: i32) -> ()");
  variant.add_global("func.func @use(%arg0: memref<8xi8>) -> memref<8xi8> { return %arg0 : memref<8xi8> }");

  variant.add_test_case_description_line("Origin: " + origin->get_name());
  variant.add_test_case_description_line("Target: " + target->get_name());
  variant.add_test_case_description_line("Bug type: " + origin_target_relation->get_printable_name() + ", linear OOBA, " + flow->get_name());
  variant.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());


  auto generate_preconditions_check_distance = std::bind(&Flow::generate_preconditions_check_distance, flow.get(), std::placeholders::_1);
  auto generate_preconditions_check_in_range = std::bind(&Flow::generate_preconditions_check_in_range, flow.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
  auto generate_counter_update = std::bind(&Flow::generate_counter_update, flow.get(), std::placeholders::_1);

  std::vector< std::shared_ptr<OriginTargetCodeCanvas> > origin_target_canvases = origin_target_relation->generate(
    variant, origin, 8, target, 8);

  for ( auto &origin_target_canvas : origin_target_canvases )
  {
    if ( origin_target_canvas->get_forces_underflow() && !is_a<Underflow>(flow) )
         continue; // the origin-target requires an underflow, but the flow is not an underflow -> skip

    // For intra-object linear OOBA, the relative position should be determined by flow:
    // Overflow -> target after origin (positive distance)
    // Underflow -> target before origin (negative distance)
    ssize_t static_dist = origin_target_canvas->get_distance_static_value();
    if ( is_a<Overflow>(flow) && static_dist < 0 ) continue;
    if ( is_a<Underflow>(flow) && static_dist > 0 ) continue;

    std::vector< std::tuple< std::string, std::string > > distance_variants;
    if ( flow->accepts_static_distance(static_dist) )
      distance_variants.push_back({ origin_target_canvas->get_distance(), "distance is checked as is" });
    if ( origin_target_canvas->get_distance_negated() != "N/A" && flow->accepts_static_distance(-static_dist) )
      distance_variants.push_back({ origin_target_canvas->get_distance_negated(), "distance is negated before checking" });
    for ( auto &distance_variant : distance_variants )
    {
      std::string distance = std::get<0>(distance_variant);
      std::string distance_description = std::get<1>(distance_variant);

      ssize_t distance_as_static_number;
      bool distance_statically_known = false;
      if ( distance == "N/A" ) continue;
      if ( is_number(distance) )
      {
        distance_as_static_number = std::stoll(distance);
        distance_statically_known = true;
        if ( !flow->accepts_static_distance(distance_as_static_number) ) continue;
      }
      auto origin_target_canvas_copy = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas);
      ssize_t static_dist = origin_target_canvas->get_distance_static_value();
      std::string origin_offset = static_dist > 0 ? "0" : std::to_string(std::abs(static_dist));
      if (!origin_target_canvas->is_target_allocated()) {
        origin_offset = std::to_string(static_dist - static_cast<ssize_t>(origin_target_canvas_copy->get_origin_size()) + 1);
      }
      std::string target_offset = static_dist > 0 ? std::to_string(static_dist) : "0";
      std::vector<AccessLocation::SplitAccess> reach_target_codes;
      if (!distance_statically_known)
      {
        {
          bool needs_strided = !is_a<NonObject>(origin_target_relation);
          reach_target_codes = access_location->generate_bulk_split_all(
              access_action, origin_target_canvas_copy->get_origin_name(), origin_target_canvas_copy->get_target_name(), distance,
              generate_preconditions_check_distance, generate_preconditions_check_in_range, generate_counter_update, needs_strided, origin_offset
            );
        }
        origin_target_canvas_copy->add_variant_description_line( distance_description );
      }
      else
      {
        // distance is statically known
        if ( distance_as_static_number == static_cast<ssize_t>(origin_target_canvas_copy->get_origin_size()) )
        {
          // special case for when there is no space in between the origin and the target
          origin_target_canvas_copy->add_variant_description_line("no space in between origin and target");
          bool needs_strided = !is_a<NonObject>(origin_target_relation);
          std::vector<AccessLocation::SplitAccess> access_target_codes = access_location->generate_split_all(
            access_action,
            origin_target_canvas_copy->get_target_name(),
            origin_target_canvas_copy->get_target_size(),generate_counter_update,distance, needs_strided, target_offset);
          for ( auto &access_target_code : access_target_codes )
          {
            auto origin_target_canvas_with_access = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas_copy);
            origin_target_canvas_with_access->add_during_lifetime(access_target_code.to_lines());
            // origin_target_canvas_with_access->add_during_lifetime("func.call @use(%" + origin_target_canvas_copy->get_origin_name() + ") : (memref<8xi8>) -> ()");
            origin_target_canvas_with_access->add_during_lifetime("func.call @exit(%test_success) : (i32) -> ()");
            origin_target_canvas_with_access->add_variant_description_line("target accessed by using " + access_target_code.description);
            full_variants.push_back(origin_target_canvas_with_access);
          }
          continue;
        }
        {
          bool needs_strided = !is_a<NonObject>(origin_target_relation);
          reach_target_codes = access_location->generate_bulk_split_all(
            access_action,origin_target_canvas_copy->get_origin_name(), origin_target_canvas_copy->get_target_name(), distance,
            generate_preconditions_check_distance, generate_preconditions_check_in_range, generate_counter_update, needs_strided, origin_offset
          );
        }
        origin_target_canvas_copy->add_variant_description_line( distance_description );
      }


      for ( auto &reach_target_code : reach_target_codes )
      {
        bool needs_strided = !is_a<NonObject>(origin_target_relation);
        std::vector<AccessLocation::SplitAccess> access_target_codes = access_location->generate_split_all(
          access_action,
          reach_target_code.result,
          origin_target_canvas_copy->get_target_size(),generate_counter_update,distance, needs_strided, origin_offset
        );
        for ( auto &access_target_code : access_target_codes )
        {
          auto origin_target_canvas_with_access = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas_copy);
          // origin_target_canvas_with_access->add_during_lifetime("func.call @use(%" + origin_target_canvas_copy->get_target_name() + ") : (memref<8xi8>) -> ()");
          // origin_target_canvas_with_access->add_during_lifetime("func.call @use(%" + origin_target_canvas_copy->get_origin_name() + ") : (memref<8xi8>) -> ()");
          origin_target_canvas_with_access->add_during_lifetime( AccessLocation::AuxiliaryVariable::to_string_vector( reach_target_code.aux_variables ) );
          origin_target_canvas_with_access->add_during_lifetime(reach_target_code.access_lines);
          origin_target_canvas_with_access->add_during_lifetime(access_target_code.to_lines());
          origin_target_canvas_with_access->add_during_lifetime("func.call @exit(%test_success) : (i32) -> ()");

          origin_target_canvas_with_access->add_variant_description_line("target reached by using a " + reach_target_code.description);
          origin_target_canvas_with_access->add_variant_description_line("target accessed by using " + access_target_code.description);

          full_variants.push_back(origin_target_canvas_with_access);
        }
      }
    }
  }

  return full_variants;
}


std::vector<std::shared_ptr<OriginTargetCodeCanvas>> LinearOOBA::generate_validation(
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
    <target, origin, aux_ptr allocations> // aux_ptr points to origin
    <action>(aux_ptr, target_size) // access the target
    return 42;
  */
  CodeCanvas variant;
  variant.add_global("func.func private @exit(%arg0: i32) -> ()");
  variant.add_global("func.func @use(%arg0: memref<8xi8>) -> memref<8xi8> { return %arg0 : memref<8xi8> }");

  auto generate_counter_update = std::bind(&Flow::generate_counter_update, flow.get(), std::placeholders::_1);

  variant.add_test_case_description_line("Origin: " + origin->get_name());
  variant.add_test_case_description_line("Target: " + target->get_name());
  variant.add_test_case_description_line("Bug type: " + origin_target_relation->get_printable_name() + ", linear OOBA, " + flow->get_name());
  variant.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  std::vector< std::shared_ptr<OriginTargetCodeCanvas> > origin_target_canvases = origin_target_relation->generate(
    variant, origin, 8, target, 8);

  for ( auto &origin_target_canvas : origin_target_canvases )
  {
    if ( origin_target_canvas->get_forces_underflow() && !is_a<Underflow>(flow) ) continue; // the origin-target requires an underflow, but the flow is not an underflow -> skip

    // For intra-object linear OOBA, the relative position should be determined by flow:
    // Overflow -> target after origin (positive distance)
    // Underflow -> target before origin (negative distance)
    ssize_t static_dist = origin_target_canvas->get_distance_static_value();
    if ( is_a<Overflow>(flow) && static_dist < 0 ) continue;
    if ( is_a<Underflow>(flow) && static_dist > 0 ) continue;

    std::string var_name_to_access;
    std::string var_offset = "0";
    if ( origin_target_canvas->is_target_allocated() )
    {
      var_name_to_access = origin_target_canvas->get_target_name();
      ssize_t static_dist = origin_target_canvas->get_distance_static_value();
      if ( static_dist > 0 )
        var_offset = std::to_string(static_dist);
    }
    else
    {
      var_name_to_access = origin_target_canvas->get_origin_name();
    }

    std::vector< std::string > distance_variants = { "0" };
    for ( auto &distance_variant : distance_variants )
    {
      if ( distance_variant == "N/A" ) continue;
      ssize_t distance_as_static_number;
      bool distance_statically_known = false;
      if ( is_number(distance_variant) )
      {
        distance_as_static_number = std::stoll(distance_variant);
        if ( !flow->accepts_static_distance(distance_as_static_number) ) continue;
        distance_statically_known = true;
      }
      auto origin_target_canvas_copy = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas);
      if ( distance_statically_known
           && distance_as_static_number == static_cast<ssize_t>( origin_target_canvas_copy->get_origin_size() )
      )
      {
        // special case for when there is no space in between the origin and the target.
        bool needs_strided = !is_a<NonObject>(origin_target_relation);
        std::vector<std::string> access_target_code = access_location->generate(
          access_action,
          var_name_to_access,
          origin_target_canvas_copy->get_target_size(), 0, "", needs_strided, var_offset);
        origin_target_canvas_copy->add_during_lifetime(access_target_code);
        // origin_target_canvas_copy->add_during_lifetime("func.call @use(%" + origin_target_canvas_copy->get_origin_name() + ") : (memref<8xi8>) -> ()");
        origin_target_canvas_copy->add_during_lifetime("func.call @exit(%test_success) : (i32) -> ()");
        origin_target_canvas_copy->add_variant_description_line("no space between origin and target");
        full_variants.push_back(origin_target_canvas_copy);
      }
      else
      {
        bool needs_strided = !is_a<NonObject>(origin_target_relation);
        AccessLocation::SplitAccess reach_target_code;
        {
          reach_target_code = access_location->generate_bulk_split_using_index(
            access_action, var_name_to_access, var_name_to_access, distance_variant,
            nullptr, nullptr, generate_counter_update, needs_strided, var_offset
          );
        }

        std::vector<std::string> access_target_code = access_location->generate(
          access_action,
          reach_target_code.result,
          origin_target_canvas_copy->get_target_size(), 0, "", needs_strided, var_offset
        );

        // if ( origin_target_canvas_copy->is_target_allocated() ) origin_target_canvas_copy->add_during_lifetime("func.call @use(%" + origin_target_canvas_copy->get_target_name() + ") : (memref<8xi8>) -> ()");
        // origin_target_canvas_copy->add_during_lifetime("func.call @use(%" + origin_target_canvas_copy->get_origin_name() + ") : (memref<8xi8>) -> ()");
        origin_target_canvas_copy->add_during_lifetime( AccessLocation::AuxiliaryVariable::to_string_vector( reach_target_code.aux_variables ) );
        origin_target_canvas_copy->add_during_lifetime(reach_target_code.access_lines);
        origin_target_canvas_copy->add_during_lifetime(access_target_code);
        origin_target_canvas_copy->add_during_lifetime("func.call @exit(%test_success) : (i32) -> ()");
        origin_target_canvas_copy->add_variant_description_line("target reached using a " + reach_target_code.description);
        full_variants.push_back(origin_target_canvas_copy);
      }
    }
    if ( !origin_target_canvas->is_target_allocated() ) break;
  }

  return full_variants;
}
