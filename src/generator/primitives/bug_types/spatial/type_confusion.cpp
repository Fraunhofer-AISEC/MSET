/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "type_confusion.h"

#include "misc.h"
#include "generator/primitives/access_types/direct_location.h"
#include "generator/primitives/bug_types/spatial/flow/overflow.h"

bool TypeConfusion::accepts(std::shared_ptr<Flow> flow) const
{
  return is_a<Overflow>(flow);
}

bool TypeConfusion::accepts(std::shared_ptr<OriginTargetRelation> origin_target_relation) const
{
  return true;
}

bool TypeConfusion::accepts(std::shared_ptr<AccessLocation> access_location) const
{
  return is_a<DirectLocation>(access_location);
}

std::vector<std::shared_ptr<OriginTargetCodeCanvas>> TypeConfusion::generate(
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
  variant.add_test_case_description_line("Bug type: " + origin_target_relation->get_printable_name() + ", type confusion OOBA, " + flow->get_name());
  variant.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());



  auto generate_preconditions_check_distance = std::bind(&Flow::generate_preconditions_check_distance, flow.get(), std::placeholders::_1);
  auto generate_preconditions_check_in_range = std::bind(&Flow::generate_preconditions_check_in_range, flow.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

  std::vector< std::shared_ptr<OriginTargetCodeCanvas> > origin_target_canvases = origin_target_relation->generate(
    variant, origin, 8, target, 8);

  for ( auto &origin_target_canvas : origin_target_canvases )
  {
    if ( origin_target_canvas->get_forces_underflow() ) continue; // skip underflows as they are incompatible with type confusions

    // simple variant with BigType
    std::shared_ptr<OriginTargetCodeCanvas> variant_with_big_type = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas);
    std::vector<std::string> reach_target_code = access_location->generate_using_runtime_index(
      access_action,
      "((struct BigType *)" + variant_with_big_type->get_origin_name() + ")->buffer",
      "i",
      variant_with_big_type->get_distance(),
      generate_preconditions_check_distance,
      generate_preconditions_check_in_range
    );
    std::vector<std::string> access_target_code = access_location->generate_at_index(
      access_action,
      "((struct BigType *)" + variant_with_big_type->get_origin_name() + ")->buffer",
      variant_with_big_type->get_distance(),
      variant_with_big_type->get_target_size(),
      nullptr
    );
    variant_with_big_type->add_during_lifetime({
      "if ( (" + variant_with_big_type->get_distance() + " > 0 && " + variant_with_big_type->get_distance() + " > ((size_t)1 << 27))",
      "     || (" + variant_with_big_type->get_distance() + " < 0 && " + variant_with_big_type->get_distance() + "< -((size_t)1 << 27) ) )"\
      "  _exit(PRECONDITIONS_FAILED_VALUE);"
    });
    variant_with_big_type->add_during_lifetime(reach_target_code);
    variant_with_big_type->add_during_lifetime(access_target_code);
    variant_with_big_type->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
    variant_with_big_type->add_global({
      "struct BigType",
      "{",
      "  char buffer[(size_t)1 << 27];",
      "};"
    });
    variant_with_big_type->add_variant_description_line("using big structure cast");

    std::shared_ptr<OriginTargetCodeCanvas> variant_with_big_type_local = std::make_shared<OriginTargetCodeCanvas>(*variant_with_big_type);
    if (variant_with_big_type->get_number_of_globals() > 0)
    {
      std::shared_ptr<OriginTargetCodeCanvas> variant_with_big_type_global_first = std::make_shared<OriginTargetCodeCanvas>(*variant_with_big_type);
      variant_with_big_type_global_first->add_global_first("static ssize_t i;");
      variant_with_big_type_global_first->add_variant_description_line("using a global index, declared first");
      full_variants.push_back( variant_with_big_type_global_first );
    }
    variant_with_big_type->add_global("static ssize_t i;");
    variant_with_big_type->add_variant_description_line("using a global index");
    full_variants.push_back( variant_with_big_type );

    variant_with_big_type_local->add_local_first("ssize_t i;");
    variant_with_big_type_local->add_variant_description_line("using a stack index");
    full_variants.push_back( variant_with_big_type_local );

    // load widening variant
    std::shared_ptr<OriginTargetCodeCanvas> variant_with_load_widening = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas);
    access_target_code = access_location->generate_uint32(
      access_action,
      variant_with_load_widening->get_origin_name(),
      variant_with_load_widening->get_origin_name(),
      variant_with_load_widening->get_distance(),
      8,
      generate_preconditions_check_distance
    );
    variant_with_load_widening->add_during_lifetime(access_target_code);
    variant_with_load_widening->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
    variant_with_load_widening->add_variant_description_line("using load widening");
    full_variants.push_back( variant_with_load_widening );
  }

  return full_variants;
}


std::vector<std::shared_ptr<OriginTargetCodeCanvas>> TypeConfusion::generate_validation(
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
  variant.add_test_case_description_line("Origin: " + origin->get_name());
  variant.add_test_case_description_line("Target: " + target->get_name());
  variant.add_test_case_description_line("Bug type: " + origin_target_relation->get_printable_name() + ", type confusion OOBA, " + flow->get_name());
  variant.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  std::vector< std::shared_ptr<OriginTargetCodeCanvas> > origin_target_canvases = origin_target_relation->generate(
    variant, origin, 8, target, 8);

  for ( auto &origin_target_canvas : origin_target_canvases )
  {
    if ( origin_target_canvas->get_forces_underflow() ) continue; // skip underflows
    // simple variant with BigType
    std::shared_ptr<OriginTargetCodeCanvas> variant_with_big_type = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas);

    std::string var_name_to_access;
    if ( origin_target_canvas->is_target_allocated() )
    {
      var_name_to_access = variant_with_big_type->get_target_name();
    }
    else
    {
      var_name_to_access = variant_with_big_type->get_origin_name();
    }

    std::vector<std::string> access_target_code = access_location->generate_at_index(
      access_action,
      var_name_to_access,
      "0",
      variant_with_big_type->get_target_size(),
      nullptr
    );
    variant_with_big_type->add_during_lifetime(access_target_code);
    variant_with_big_type->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
    variant_with_big_type->add_global({
      "struct BigType",
      "{",
      "  char buffer[(size_t)1 << 27];",
      "};"
    });
    variant_with_big_type->add_variant_description_line("using big structure cast");
    full_variants.push_back( variant_with_big_type );

    // load widening variant
    std::shared_ptr<OriginTargetCodeCanvas> variant_with_load_widening = std::make_shared<OriginTargetCodeCanvas>(*origin_target_canvas);
    access_target_code = access_location->generate_uint8(
      access_action,
      variant_with_load_widening->get_origin_name(),
      variant_with_load_widening->get_origin_name(),
      variant_with_load_widening->get_distance(),
      4,
      nullptr
    );
    variant_with_load_widening->add_during_lifetime(access_target_code);
    variant_with_load_widening->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
    variant_with_load_widening->add_variant_description_line("using load widening");
    full_variants.push_back( variant_with_load_widening );
  }

  return full_variants;
}
