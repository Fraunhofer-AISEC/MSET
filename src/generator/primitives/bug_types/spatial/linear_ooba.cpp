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
    if ( origin_target_canvas->get_forces_underflow() && !is_a<Underflow>(flow) ) continue; // the origin-target requires an underflow, but the for is not an underflow -> skip

    std::vector< std::tuple< std::string, std::string > > distance_variants = {
      std::tuple< std::string, std::string >{ origin_target_canvas->get_distance(), "distance is checked as it is" },
      std::tuple< std::string, std::string >{ origin_target_canvas->get_distance_negated(), "distance is negated before checking" }
    };
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
      std::vector<AccessLocation::SplitAccess> reach_target_codes;
      if (!distance_statically_known)
      {
        reach_target_codes = access_location->generate_bulk_split_all(
            access_action, origin_target_canvas_copy->get_origin_name(), origin_target_canvas_copy->get_target_name(), distance,
            generate_preconditions_check_distance, generate_preconditions_check_in_range, generate_counter_update
          );
        origin_target_canvas_copy->add_to_variant_description( distance_description );
      }
      else
      {
        // distance is statically known
        if ( distance_as_static_number == static_cast<ssize_t>(origin_target_canvas_copy->get_origin_size()) )
        {
          // special case for when there is no space in between the origin and the target
          origin_target_canvas_copy->add_to_variant_description("no space in between origin and target");
          std::vector<AccessLocation::SplitAccess> access_target_codes = access_location->generate_split_all(
            access_action,
            "(" + origin_target_canvas_copy->get_origin_name() + " + " + std::to_string(origin_target_canvas_copy->get_target_size()) + ")",
            distance_as_static_number);
          for ( auto &access_target_code : access_target_codes )
          {
            auto origin_target_canvas_with_access = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas_copy);
            origin_target_canvas_with_access->add_during_lifetime(access_target_code.to_lines());
            origin_target_canvas_with_access->add_during_lifetime("_use(" + origin_target_canvas_copy->get_origin_name() + ");");
            origin_target_canvas_with_access->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
            origin_target_canvas_with_access->add_to_variant_description("target accessed by " + access_target_code.description);
            full_variants.push_back(origin_target_canvas_with_access);
          }
          continue;
        }
        reach_target_codes = access_location->generate_bulk_split_all(
          access_action,origin_target_canvas_copy->get_origin_name(), origin_target_canvas_copy->get_target_name(), distance,
          generate_preconditions_check_distance, generate_preconditions_check_in_range, generate_counter_update
        );
        origin_target_canvas_copy->add_to_variant_description( distance_description );
      }


      for ( auto &reach_target_code : reach_target_codes )
      {
        std::vector<AccessLocation::SplitAccess> access_target_codes = access_location->generate_split_all(
          access_action,
          reach_target_code.result,
          origin_target_canvas_copy->get_target_size()
        );
        for ( auto &access_target_code : access_target_codes )
        {
          auto origin_target_canvas_with_access = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas_copy);
          origin_target_canvas_with_access->add_to_variant_description("target reached by " + reach_target_code.description);
          origin_target_canvas_with_access->add_during_lifetime("_use(" + origin_target_canvas_copy->get_target_name() + ");");
          origin_target_canvas_with_access->add_during_lifetime("_use(" + origin_target_canvas_copy->get_origin_name() + ");");
          origin_target_canvas_with_access->add_during_lifetime(reach_target_code.access_lines);
          origin_target_canvas_with_access->add_during_lifetime(access_target_code.to_lines());
          origin_target_canvas_with_access->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
          origin_target_canvas_with_access->add_to_variant_description("target accessed by " + access_target_code.description);
          if ( is_a<ReadAction>( access_action ) )
          {
            // since reading cannot corrupt, just allocate the aux variables as globals
            std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_global_variant = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas_with_access);
            aux_ptr_global_variant->add_globals( AccessLocation::AuxiliaryVariable::to_string_vector( reach_target_code.aux_variables ) );

            full_variants.push_back(aux_ptr_global_variant);
          }
          else
          {
            std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_global_last_variant = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas_with_access);
            aux_ptr_global_last_variant->add_globals( AccessLocation::AuxiliaryVariable::to_string_vector( reach_target_code.aux_variables ) );
            aux_ptr_global_last_variant->add_to_variant_description("global auxiliary variables, declared last");
            full_variants.push_back(aux_ptr_global_last_variant);

            if (origin_target_canvas_with_access->get_number_of_globals() > 0)
            {
              std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_global_first_variant = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas_with_access);
              aux_ptr_global_first_variant->add_globals_first( AccessLocation::AuxiliaryVariable::to_string_vector( reach_target_code.aux_variables ) );
              aux_ptr_global_first_variant->add_to_variant_description("global auxiliary variables, declared first");
              full_variants.push_back(aux_ptr_global_first_variant);
            }
            // else there is no difference between add_global and add_global_first


            std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_global_init_last_variant = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas_with_access);
            std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_global_init_first_variant = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas_with_access);
            bool global_init_added = false;
            for ( const AccessLocation::AuxiliaryVariable& aux_variable: reach_target_code.aux_variables )
            {
              if ( aux_variable.init_value.empty() )
              {
                // initialize this variable
                AccessLocation::AuxiliaryVariable aux_variable_init = aux_variable;
                aux_variable_init.init_value = "0";
                aux_ptr_global_init_last_variant->add_global( aux_variable_init.to_string() );
                aux_ptr_global_init_first_variant->add_global_first( aux_variable_init.to_string() );
                global_init_added = true;
              }
              else
              {
                aux_ptr_global_init_last_variant->add_global( aux_variable.to_string() );
                aux_ptr_global_init_first_variant->add_global_first( aux_variable.to_string() );
              }
            }
            if (global_init_added)
            {
              aux_ptr_global_init_last_variant->add_to_variant_description("global auxiliary variables, initialized, declared last");
              full_variants.push_back(aux_ptr_global_init_last_variant);
              if (origin_target_canvas_with_access->get_number_of_globals() > 0)
              {
                aux_ptr_global_init_first_variant->add_to_variant_description("global auxiliary variables, initialized, declared first");
                full_variants.push_back(aux_ptr_global_init_first_variant);
              }
              // else there is no difference between add_global and add_global_first
            }


            std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_stack_last_variant = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas_with_access);
            aux_ptr_stack_last_variant->add_locals( AccessLocation::AuxiliaryVariable::to_string_vector( reach_target_code.aux_variables ) );
            aux_ptr_stack_last_variant->add_to_variant_description("stack auxiliary variables, declared last");
            full_variants.push_back(aux_ptr_stack_last_variant);

            if (origin_target_canvas_with_access->get_number_of_locals() > 0)
            {
              std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_stack_first_variant = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas_with_access);
              aux_ptr_stack_first_variant->add_locals_first( AccessLocation::AuxiliaryVariable::to_string_vector( reach_target_code.aux_variables ) );
              aux_ptr_stack_first_variant->add_to_variant_description("stack auxiliary variables, declared first");
              full_variants.push_back(aux_ptr_stack_first_variant);
            }
            // else there is no difference

          }
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
    _exit(TEST_CASE_SUCCESSFUL_VALUE);
  */
  CodeCanvas variant;
  auto generate_counter_update = std::bind(&Flow::generate_counter_update, flow.get(), std::placeholders::_1);

  variant.add_test_case_description_line("Origin: " + origin->get_name());
  variant.add_test_case_description_line("Target: " + target->get_name());
  variant.add_test_case_description_line("Bug type: " + origin_target_relation->get_printable_name() + ", linear OOBA, " + flow->get_name());
  variant.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

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
        if ( !flow->accepts_static_distance(distance_as_static_number) ) continue;
        distance_statically_known = true;
      }
      auto origin_target_canvas_copy = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas);
      AccessLocation::SplitAccess reach_target_code;
      if (!distance_statically_known)
      {
        reach_target_code = access_location->generate_bulk_split_using_index(
          access_action, var_name_to_access, var_name_to_access, distance_variant,
          nullptr, nullptr, generate_counter_update
        );
        origin_target_canvas_copy->add_to_variant_description("target reached by " + reach_target_code.description);
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
          origin_target_canvas_copy->add_to_variant_description("no space between origin and target");
          full_variants.push_back(origin_target_canvas_copy);
          continue;
        }
        reach_target_code = access_location->generate_bulk_split_using_index(
          access_action, var_name_to_access, var_name_to_access, distance_variant,
          nullptr, nullptr, generate_counter_update
        );
        origin_target_canvas_copy->add_to_variant_description("target reached by " + reach_target_code.description);
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
      if ( is_a<ReadAction>(access_action) )
      {
        // since reading cannot corrupt, just allocate the aux variables as globals
        std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_global_variant = std::make_shared<OriginTargetCodeCanvas>(
          *origin_target_canvas_copy);
        aux_ptr_global_variant->add_globals( AccessLocation::AuxiliaryVariable::to_string_vector( reach_target_code.aux_variables ) );
        aux_ptr_global_variant->add_to_variant_description("global auxiliary variables");
        full_variants.push_back(aux_ptr_global_variant);
      }
      else
      {
        std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_global_last_variant = std::make_shared<OriginTargetCodeCanvas>(
          *origin_target_canvas_copy);
        aux_ptr_global_last_variant->add_globals( AccessLocation::AuxiliaryVariable::to_string_vector( reach_target_code.aux_variables ) );
        aux_ptr_global_last_variant->add_to_variant_description("global auxiliary variables, declared last");
        full_variants.push_back(aux_ptr_global_last_variant);

        if (origin_target_canvas_copy->get_number_of_globals() > 0)
        {
          std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_global_first_variant = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas_copy);
          aux_ptr_global_first_variant->add_globals_first( AccessLocation::AuxiliaryVariable::to_string_vector( reach_target_code.aux_variables ) );
          aux_ptr_global_first_variant->add_to_variant_description("global auxiliary variables, declared first");
          full_variants.push_back(aux_ptr_global_first_variant);
        }
        // else there is no difference between add_global and add_global_first


        std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_global_init_last_variant = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas_copy);
        std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_global_init_first_variant = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas_copy);
        bool global_init_added = false;
        for ( const AccessLocation::AuxiliaryVariable& aux_variable: reach_target_code.aux_variables )
        {
          if ( aux_variable.init_value.empty() )
          {
            // initialize this variable
            AccessLocation::AuxiliaryVariable aux_variable_init = aux_variable;
            aux_variable_init.init_value = "0";
            aux_ptr_global_init_last_variant->add_global( aux_variable_init.to_string() );
            aux_ptr_global_init_first_variant->add_global_first( aux_variable_init.to_string() );
            global_init_added = true;
          }
          else
          {
            aux_ptr_global_init_last_variant->add_global( aux_variable.to_string() );
            aux_ptr_global_init_first_variant->add_global_first( aux_variable.to_string() );
          }
        }
        if (global_init_added)
        {
          aux_ptr_global_init_last_variant->add_to_variant_description("global auxiliary variables, declared last");
          full_variants.push_back(aux_ptr_global_init_last_variant);
          if (origin_target_canvas_copy->get_number_of_globals() > 0)
          {
            aux_ptr_global_init_first_variant->add_to_variant_description("global auxiliary variables, declared first");
            full_variants.push_back(aux_ptr_global_init_first_variant);
          }
          // else there is no difference between add_global and add_global_first
        }


        std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_stack_last_variant = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas_copy);
        aux_ptr_stack_last_variant->add_locals( AccessLocation::AuxiliaryVariable::to_string_vector( reach_target_code.aux_variables ) );
        aux_ptr_stack_last_variant->add_to_variant_description("stack auxiliary variables, declared last");

        full_variants.push_back(aux_ptr_stack_last_variant);

        if (origin_target_canvas_copy->get_number_of_locals() > 0)
        {
          std::shared_ptr<OriginTargetCodeCanvas> aux_ptr_stack_first_variant = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas_copy);
          aux_ptr_stack_first_variant->add_locals_first( AccessLocation::AuxiliaryVariable::to_string_vector( reach_target_code.aux_variables ) );
          aux_ptr_stack_first_variant->add_to_variant_description("stack auxiliary variables, declared first");

          full_variants.push_back(aux_ptr_stack_first_variant);
        }
      }
    }
  }

  return full_variants;
}
