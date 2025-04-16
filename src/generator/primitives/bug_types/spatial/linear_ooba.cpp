/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "linear_ooba.h"

#include <cassert>

#include "misc.h"
#include "generator/primitives/access_types/direct_location.h"
#include "generator/primitives/access_types/read_action.h"
#include "generator/primitives/bug_types/spatial/flow/overflow.h"
#include "generator/primitives/bug_types/spatial/flow/underflow.h"
#include "generator/primitives/bug_types/spatial/origin_target_relation/intra_object.h"

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
    _exit(TEST_CASE_SUCCESSFUL_VALUE);
  */
  CodeCanvas variant;


  auto generate_preconditions_check_distance = std::bind(&Flow::generate_preconditions_check_distance, flow.get(), std::placeholders::_1);
  auto generate_preconditions_check_in_range = std::bind(&Flow::generate_preconditions_check_in_range, flow.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
  auto generate_counter_update = std::bind(&Flow::generate_counter_update, flow.get(), std::placeholders::_1);

  std::vector< std::shared_ptr<OriginTargetCodeCanvas> > origin_target_canvases = origin_target_relation->generate(
    variant, origin, 8, target, 8);

  for ( auto &origin_target_canvas : origin_target_canvases )
  {
    if ( origin_target_canvas->get_forces_underflow() && !is_a<Underflow>(flow) ) continue; // the origin-target requires an underflow, but the for is not an underflow -> skip

    std::vector< std::string > distance_variants = { origin_target_canvas->get_distance(), origin_target_canvas->get_distance_negated() };
    for ( auto &distance_variant : distance_variants )
    {
      ssize_t distance_as_static_number;
      bool distance_statically_known = false;
      if ( distance_variant == "N/A" ) continue;
      if ( is_number(distance_variant) )
      {
        distance_as_static_number = std::stoll(distance_variant);
        distance_statically_known = true;
        if ( !flow->accepts_static_distance(distance_as_static_number) ) continue;
      }
      auto origin_target_canvas_copy = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas);
      AccessLocation::SplitAccessLines reach_target_code;
      if (!distance_statically_known)
      {
        if (is_a<IntraObject>(origin_target_relation)
            && is_a<Overflow>(flow)
            && is_a<ReadAction>(access_action)
            && is_a<DirectLocation>(access_location) )
        {
          std::vector<std::string> lines = access_location->generate_with_runtime_index(
            access_action, origin_target_canvas_copy->get_origin_name(), "index", distance_variant,
            generate_preconditions_check_distance, generate_preconditions_check_in_range
          );
          reach_target_code.result = "(" + origin_target_canvas_copy->get_origin_name() + " + index)";
          reach_target_code.access_lines = {"size_t index = 0;"};
          reach_target_code.access_lines.insert( reach_target_code.access_lines.end(), lines.begin(), lines.end() );
        }
        else
        {
          reach_target_code = access_location->generate_bulk_split(
            access_action, origin_target_canvas_copy->get_origin_name(), origin_target_canvas_copy->get_target_name(), distance_variant,
            generate_preconditions_check_distance, generate_preconditions_check_in_range, generate_counter_update
          );
        }
      }
      else
      {
        // distance is statically known
        if ( distance_as_static_number == static_cast<ssize_t>(origin_target_canvas_copy->get_origin_size()) )
        {
          // special case for when there is no space in between the origin and the target
          std::vector<std::string> access_target_code = access_location->generate(
            access_action,
            "(" + origin_target_canvas_copy->get_origin_name() + " + " + std::to_string(origin_target_canvas_copy->get_target_size()) + ")",
            distance_as_static_number);
          origin_target_canvas_copy->add_during_lifetime(access_target_code);
          origin_target_canvas_copy->add_during_lifetime("_use(" + origin_target_canvas_copy->get_origin_name() + ");");
          origin_target_canvas_copy->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
          full_variants.push_back(origin_target_canvas_copy);
          continue;
        }
        reach_target_code = access_location->generate_bulk_split(
          access_action,origin_target_canvas_copy->get_origin_name(), origin_target_canvas_copy->get_target_name(), distance_as_static_number,
          generate_preconditions_check_in_range, generate_counter_update
        );
      }

      std::vector<std::string> access_target_code = access_location->generate(
        access_action,
        reach_target_code.result,
        origin_target_canvas_copy->get_target_size()
      );


      origin_target_canvas_copy->add_during_lifetime("_use(" + origin_target_canvas_copy->get_target_name() + ");");
      origin_target_canvas_copy->add_during_lifetime("_use(" + origin_target_canvas_copy->get_origin_name() + ");");
      origin_target_canvas_copy->add_during_lifetime(reach_target_code.access_lines);
      origin_target_canvas_copy->add_during_lifetime(access_target_code);
      origin_target_canvas_copy->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
      if (reach_target_code.aux_variable_needs_allocation)
      {
        assert(!reach_target_code.aux_variable.empty());
        std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_global_last_variant = std::make_shared<OriginTargetCodeCanvas>(
          *origin_target_canvas_copy);
        aux_ptr_global_last_variant->add_global(reach_target_code.aux_variable_type + " " + reach_target_code.aux_variable + ";");
        full_variants.push_back(aux_ptr_global_last_variant);

        if (origin_target_canvas_copy->get_number_of_globals() > 0)
        {
          std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_global_first_variant = std::make_shared<
            OriginTargetCodeCanvas>(*origin_target_canvas_copy);
          aux_ptr_global_first_variant->add_global_first(reach_target_code.aux_variable_type + " " + reach_target_code.aux_variable + ";");
          full_variants.push_back(aux_ptr_global_first_variant);
        }
        // else there is no difference between add_global and add_global_first

        std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_global_init_last_variant = std::make_shared<
          OriginTargetCodeCanvas>(*origin_target_canvas_copy);
        aux_ptr_global_init_last_variant->add_global(reach_target_code.aux_variable_type + " " + reach_target_code.aux_variable + " = 0;");
        full_variants.push_back(aux_ptr_global_init_last_variant);

        if (origin_target_canvas_copy->get_number_of_globals() > 0)
        {
          std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_global_init_first_variant = std::make_shared<
            OriginTargetCodeCanvas>(*origin_target_canvas_copy);
          aux_ptr_global_init_first_variant->add_global_first(reach_target_code.aux_variable_type + " " + reach_target_code.aux_variable + " = 0;");
          full_variants.push_back(aux_ptr_global_init_first_variant);
        }
        // else there is no difference between add_global and add_global_first

        std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_stack_last_variant = std::make_shared<OriginTargetCodeCanvas>(
          *origin_target_canvas_copy);
        aux_ptr_stack_last_variant->add_local(reach_target_code.aux_variable_type + " " + reach_target_code.aux_variable + ";");
        full_variants.push_back(aux_ptr_stack_last_variant);

        if (origin_target_canvas_copy->get_number_of_locals() > 0)
        {
          std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_stack_first_variant = std::make_shared<
            OriginTargetCodeCanvas>(*origin_target_canvas_copy);
          aux_ptr_stack_first_variant->add_local_first(reach_target_code.aux_variable_type + " " + reach_target_code.aux_variable + ";");
          full_variants.push_back(aux_ptr_stack_first_variant);
        }
      } else
      {
        full_variants.push_back(origin_target_canvas_copy);
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
    _exit(TEST_CASE_SUCCESSFUL_VALUE);
  */
  CodeCanvas variant;
  auto generate_counter_update = std::bind(&Flow::generate_counter_update, flow.get(), std::placeholders::_1);

  std::vector< std::shared_ptr<OriginTargetCodeCanvas> > origin_target_canvases = origin_target_relation->generate(
    variant, origin, 8, target, 8);

  for ( auto &origin_target_canvas : origin_target_canvases )
  {
    if ( origin_target_canvas->get_forces_underflow() && !is_a<Underflow>(flow) ) continue; // the origin-target requires an underflow, but the for is not an underflow -> skip

    std::string var_name_to_access;
    if ( origin_target_canvas->is_target_allocated() )
    {
      var_name_to_access = origin_target_canvas->get_target_name();
    }
    else
    {
      var_name_to_access = origin_target_canvas->get_origin_name();
    }

    std::vector< std::string > distance_variants = { "0" };
    for ( auto &distance_variant : distance_variants )
    {
      ssize_t distance_as_static_number;
      bool distance_statically_known = false;
      if ( distance_variant == "N/A" ) continue;
      if ( is_number(distance_variant) )
      {
        distance_as_static_number = std::stoll(distance_variant);
        distance_statically_known = true;
        if ( !flow->accepts_static_distance(distance_as_static_number) ) continue;
      }
      auto origin_target_canvas_copy = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas);
      AccessLocation::SplitAccessLines reach_target_code;
      if (!distance_statically_known)
      {
        if (is_a<IntraObject>(origin_target_relation)
            && is_a<Overflow>(flow)
            && is_a<ReadAction>(access_action)
            && is_a<DirectLocation>(access_location) )
        {
          std::vector<std::string> lines = access_location->generate_with_runtime_index(
            access_action, var_name_to_access, "index", distance_variant,
            nullptr, nullptr
          );
          reach_target_code.result = "(" + var_name_to_access + " + index)";
          reach_target_code.access_lines = {"size_t index = 0;"};
          reach_target_code.access_lines.insert( reach_target_code.access_lines.end(), lines.begin(), lines.end() );
        }
        else
        {
          reach_target_code = access_location->generate_bulk_split(
            access_action, var_name_to_access, var_name_to_access, distance_variant,
            nullptr, nullptr, generate_counter_update
          );
        }
      }
      else
      {
        // distance is statically known
        if ( distance_as_static_number == static_cast<ssize_t>(origin_target_canvas_copy->get_origin_size()) )
        {
          // special case for when there is no space in between the origin and the target.
          std::vector<std::string> access_target_code = access_location->generate(
            access_action,
            "(" + var_name_to_access + " + " + std::to_string(origin_target_canvas_copy->get_target_size()) + ")",
            distance_as_static_number);
          origin_target_canvas_copy->add_during_lifetime(access_target_code);
          origin_target_canvas_copy->add_during_lifetime("_use(" + origin_target_canvas_copy->get_origin_name() + ");");
          origin_target_canvas_copy->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
          full_variants.push_back(origin_target_canvas_copy);
          continue;
        }
        reach_target_code = access_location->generate_bulk_split(
          access_action, var_name_to_access, var_name_to_access, distance_as_static_number,
          nullptr, generate_counter_update
        );
      }

      std::vector<std::string> access_target_code = access_location->generate(
        access_action,
        reach_target_code.result,
        origin_target_canvas_copy->get_target_size()
      );


      if ( origin_target_canvas_copy->is_target_allocated() ) origin_target_canvas_copy->add_during_lifetime("_use(" + origin_target_canvas_copy->get_target_name() + ");");
      origin_target_canvas_copy->add_during_lifetime("_use(" + origin_target_canvas_copy->get_origin_name() + ");");
      origin_target_canvas_copy->add_during_lifetime(reach_target_code.access_lines);
      origin_target_canvas_copy->add_during_lifetime(access_target_code);
      origin_target_canvas_copy->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
      if (reach_target_code.aux_variable_needs_allocation)
      {
        assert(!reach_target_code.aux_variable.empty());
        std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_global_last_variant = std::make_shared<OriginTargetCodeCanvas>(
          *origin_target_canvas_copy);
        aux_ptr_global_last_variant->add_global(reach_target_code.aux_variable_type + " " + reach_target_code.aux_variable + ";");
        full_variants.push_back(aux_ptr_global_last_variant);

        if (origin_target_canvas_copy->get_number_of_globals() > 0)
        {
          std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_global_first_variant = std::make_shared<
            OriginTargetCodeCanvas>(*origin_target_canvas_copy);
          aux_ptr_global_first_variant->add_global_first(reach_target_code.aux_variable_type + " " + reach_target_code.aux_variable + ";");
          full_variants.push_back(aux_ptr_global_first_variant);
        }
        // else there is no difference between add_global and add_global_first

        std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_global_init_last_variant = std::make_shared<
          OriginTargetCodeCanvas>(*origin_target_canvas_copy);
        aux_ptr_global_init_last_variant->add_global(reach_target_code.aux_variable_type + " " + reach_target_code.aux_variable + " = 0;");
        full_variants.push_back(aux_ptr_global_init_last_variant);

        if (origin_target_canvas_copy->get_number_of_globals() > 0)
        {
          std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_global_init_first_variant = std::make_shared<
            OriginTargetCodeCanvas>(*origin_target_canvas_copy);
          aux_ptr_global_init_first_variant->add_global_first(reach_target_code.aux_variable_type + " " + reach_target_code.aux_variable + " = 0;");
          full_variants.push_back(aux_ptr_global_init_first_variant);
        }
        // else there is no difference between add_global and add_global_first

        std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_stack_last_variant = std::make_shared<OriginTargetCodeCanvas>(
          *origin_target_canvas_copy);
        aux_ptr_stack_last_variant->add_local(reach_target_code.aux_variable_type + " " + reach_target_code.aux_variable + ";");
        full_variants.push_back(aux_ptr_stack_last_variant);

        if (origin_target_canvas_copy->get_number_of_locals() > 0)
        {
          std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_stack_first_variant = std::make_shared<
            OriginTargetCodeCanvas>(*origin_target_canvas_copy);
          aux_ptr_stack_first_variant->add_local_first(reach_target_code.aux_variable_type + " " + reach_target_code.aux_variable + ";");
          full_variants.push_back(aux_ptr_stack_first_variant);
        }
      } else
      {
        full_variants.push_back(origin_target_canvas_copy);
      }
    }
  }

  return full_variants;
}
