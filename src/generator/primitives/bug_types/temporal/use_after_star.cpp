/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "use_after_star.h"

#include <cassert>

#include "misc.h"
#include "generator/primitives/bug_types/temporal/memory_state/freed_memory.h"
#include "generator/primitives/bug_types/temporal/memory_state/used.h"
#include "generator/primitives/regions/heap_region.h"
#include "generator/primitives/regions/stack_region.h"

const std::string max_reallocated_retries = "1000000000";
const std::string max_reallocated_retries_validation = "100";

UseAfterStar::UseAfterStar():
  TemporalBugType("use_after_star")
{
}

bool UseAfterStar::accepts(std::shared_ptr<MemoryState> memory_state)
{
  return true;
}

bool UseAfterStar::accepts(std::shared_ptr<Region> region)
{
  return is_a<HeapRegion>(region)
    || is_a<StackRegion>(region);
}

bool UseAfterStar::accepts(std::shared_ptr<AccessLocation> access_location)
{
  return true;
}

std::vector< std::shared_ptr<RegionCodeCanvas> >UseAfterStar::generate(
  std::shared_ptr<MemoryState> memory_state,
  std::shared_ptr<Region> memory_region,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location
  ) const
{
  if (is_a<FreedMemory>(memory_state))
  {
    return _generate_unused_mem(memory_region, access_action, access_location);
  }
  assert(is_a<UsedMemory>(memory_state));
  return _generate_reused_mem(memory_region, access_action, access_location);
}

std::vector< std::shared_ptr<RegionCodeCanvas> >UseAfterStar::_generate_unused_mem(
  std::shared_ptr<Region> memory_region,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location) const
{
  if ( is_a<HeapRegion>(memory_region) ) return _generate_unused_mem_heap(memory_region, access_action, access_location);
  assert( is_a<StackRegion>(memory_region) );
  return _generate_unused_mem_stack(memory_region, access_action, access_location);
}

std::vector< std::shared_ptr<RegionCodeCanvas> >UseAfterStar::_generate_unused_mem_stack(
  std::shared_ptr<Region> memory_region,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location) const
{

  std::vector< std::shared_ptr<RegionCodeCanvas> >variants = {};
  CodeCanvas code_simple;
  code_simple.add_global("char *target_address;");
  std::shared_ptr<StackRegion> stack_memory_region = std::dynamic_pointer_cast<StackRegion>(memory_region);
  std::shared_ptr<StackRegion> stack_memory_region_simple = std::make_shared<StackRegion>(*stack_memory_region);
  std::shared_ptr<RegionCodeCanvas> region_canvas = stack_memory_region_simple->generate(std::make_shared<CodeCanvas>(code_simple), "target", 8, true);

  region_canvas->add_during_lifetime("target_address = &target[0];");
  AccessLocation::SplitAccess access_type_code = access_location->generate_split_const_vars(
    access_action, "target_address", 8);

  region_canvas->add_to_main_body(access_type_code.access_lines);
  region_canvas->add_to_main_body("_exit(TEST_CASE_SUCCESSFUL_VALUE);");

  region_canvas->add_globals( AccessLocation::AuxiliaryVariable::to_string_vector( access_type_code.aux_variables ) );

  region_canvas->add_test_case_description_line("Memory region: stack");
  region_canvas->add_test_case_description_line("Bug type: use-after-*, freed memory");
  region_canvas->add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  variants.push_back( region_canvas );

  return variants;
}


std::vector< std::shared_ptr<RegionCodeCanvas> >UseAfterStar::_generate_unused_mem_heap(
  std::shared_ptr<Region> memory_region,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location) const
{
  std::vector< std::shared_ptr<RegionCodeCanvas> >variants;
  CodeCanvas code;
  code.add_global("char *target_address;");
  std::shared_ptr<RegionCodeCanvas> region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(code), "target", 8, /*initialize=*/true);

  assert( is_a<HeapRegion>(memory_region) );
  std::vector<std::string> access_type_code = access_location->generate(
    access_action, "target_address", 8);
  auto index = region_canvas->add_at(region_canvas->get_deallocation_pos(), "target_address = &target[0];", "  ");
  // region_canvas->add_during_lifetime("target_address = &target[0];");
  index = region_canvas->add_at(index, access_type_code, "  ");
  region_canvas->add_at(index, "_exit(TEST_CASE_SUCCESSFUL_VALUE);", "  ");

  region_canvas->add_test_case_description_line("Memory region: heap");
  region_canvas->add_test_case_description_line("Bug type: use-after-*, freed memory");
  region_canvas->add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  variants.push_back( region_canvas );

  return variants;
}


std::vector< std::shared_ptr<RegionCodeCanvas> >UseAfterStar::_generate_reused_mem(
  std::shared_ptr<Region> memory_region,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location) const
{
  if ( is_a<HeapRegion>(memory_region) ) return _generate_reused_mem_heap(memory_region, access_action, access_location);
  assert( is_a<StackRegion>(memory_region) );
  return _generate_reused_mem_stack(memory_region, access_action, access_location);
}

std::vector< std::shared_ptr<RegionCodeCanvas> >UseAfterStar::_generate_reused_mem_heap(
  std::shared_ptr<Region> memory_region,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location) const
{
  std::vector< std::shared_ptr<RegionCodeCanvas> >variants;
  CodeCanvas code;
  code.add_global("char *target_address;");
  std::shared_ptr<RegionCodeCanvas> region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(code), "target", 8, false);

  assert ( is_a<HeapRegion>(memory_region) );
  region_canvas->add_during_lifetime("target_address = &target[0];");
    std::vector<std::string> access_type_code = access_location->generate(
    access_action, "target_address", 8);
  std::shared_ptr<HeapRegion> heap_memory_region = std::dynamic_pointer_cast<HeapRegion>(memory_region);

  std::shared_ptr<RegionCodeCanvas> reused_region_canvas = heap_memory_region->generate(
    region_canvas->get_deallocation_pos(), region_canvas, "reallocated", 8, false);
  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_simple = std::make_shared<RegionCodeCanvas>(*reused_region_canvas);

  reused_region_canvas_simple->add_during_lifetime(
    "if ( GET_ADDR_BITS(target) != GET_ADDR_BITS(reallocated) ) _exit(PRECONDITIONS_FAILED_VALUE);"
  );
  reused_region_canvas_simple->add_during_lifetime(access_type_code);
  reused_region_canvas_simple->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");

  region_canvas->add_test_case_description_line("Memory region: heap");
  region_canvas->add_test_case_description_line("Bug type: use-after-*, reused memory");
  region_canvas->add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_repeat = std::make_shared<RegionCodeCanvas>(*reused_region_canvas);
  std::vector<std::string> allocation = heap_memory_region->generate_reallocation("reallocated", 8, true, "  ");
  std::vector<std::string> deallocation = heap_memory_region->generate_deallocation("reallocated", "  ");
  reused_region_canvas_repeat->add_during_lifetime({
    "size_t counter = 0;",
    "while ( counter < " + max_reallocated_retries + ")",
    "{",
  });
  reused_region_canvas_repeat->add_during_lifetime(deallocation);
  reused_region_canvas_repeat->add_during_lifetime(allocation);
  reused_region_canvas_repeat->add_during_lifetime({
    "  if ( GET_ADDR_BITS(target) == GET_ADDR_BITS(reallocated) ) break;",
    "  counter++;"
    // "  last_address = reallocated;",
    "}",
    "if ( counter == " + max_reallocated_retries + " ) _exit(PRECONDITIONS_FAILED_VALUE);",
  });
  reused_region_canvas_repeat->add_during_lifetime(access_type_code);
  reused_region_canvas_repeat->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");

  reused_region_canvas_repeat->add_variant_description_line("with repeated attempts");

  variants = {reused_region_canvas_simple, reused_region_canvas_repeat};

  return variants;
}

std::vector< std::shared_ptr<RegionCodeCanvas> >UseAfterStar::_generate_reused_mem_stack(
  std::shared_ptr<Region> memory_region,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location) const
{
  std::vector< std::shared_ptr<RegionCodeCanvas> >variants = {};
  CodeCanvas code_simple;
  code_simple.add_global("char *target_address;");
  std::shared_ptr<StackRegion> stack_memory_region = std::dynamic_pointer_cast<StackRegion>(memory_region);
  std::shared_ptr<StackRegion> stack_memory_region_simple = std::make_shared<StackRegion>(*stack_memory_region);
  std::shared_ptr<RegionCodeCanvas> region_canvas = stack_memory_region_simple->generate(std::make_shared<CodeCanvas>(code_simple), "target", 8, true);
  region_canvas->add_during_lifetime("target_address = &target[0];");
  AccessLocation::SplitAccess access_type_code = access_location->generate_split_const_vars(
    access_action, "target_address", 8);

  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_simple = stack_memory_region_simple->generate_in_other_f(
    region_canvas, "reallocated", 8, false
  );
  reused_region_canvas_simple->add_during_lifetime("if (GET_ADDR_BITS(&reallocated[0]) != GET_ADDR_BITS(target_address)) _exit(PRECONDITIONS_FAILED_VALUE);");
  reused_region_canvas_simple->add_during_lifetime(access_type_code.access_lines);
  reused_region_canvas_simple->add_during_lifetime("_use(reallocated);");
  reused_region_canvas_simple->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
  reused_region_canvas_simple->add_globals( AccessLocation::AuxiliaryVariable::to_string_vector( access_type_code.aux_variables ) );

  region_canvas->add_test_case_description_line("Memory region: stack");
  region_canvas->add_test_case_description_line("Bug type: use-after-*, reused memory");
  region_canvas->add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  variants.push_back( reused_region_canvas_simple );

  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_repeated = stack_memory_region_simple->generate_in_other_f(
    region_canvas, "reallocated", 8, false
    );
  reused_region_canvas_repeated->add_global("int counter = 0;");
  reused_region_canvas_repeated->add_global("char *last_address = NULL;");
  reused_region_canvas_repeated->add_during_lifetime({
    "if (GET_ADDR_BITS(last_address) == GET_ADDR_BITS(&reallocated[0])) _exit(PRECONDITIONS_FAILED_VALUE); // repeating does not help",
    "last_address = &reallocated[0];",
    "if (GET_ADDR_BITS(&reallocated[0]) != GET_ADDR_BITS(target_address)) return PRECONDITIONS_FAILED_VALUE;"
  });
  reused_region_canvas_repeated->add_during_lifetime(access_type_code.access_lines);
  reused_region_canvas_repeated->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
  reused_region_canvas_repeated->add_globals( AccessLocation::AuxiliaryVariable::to_string_vector( access_type_code.aux_variables ) );
  reused_region_canvas_repeated->prefix_line_with(reused_region_canvas_repeated->get_other_f_call_pos(), "    (void)");
  reused_region_canvas_repeated->add_at(reused_region_canvas_repeated->get_other_f_call_pos(),
    std::vector<std::string>{
      "do",
      "{"
    },
    "  "
  );
  reused_region_canvas_repeated->add_at(reused_region_canvas_repeated->get_other_f_call_pos() + 1,
    std::vector<std::string>{
      "} while (counter++ < " + max_reallocated_retries + ");",
      "_exit(PRECONDITIONS_FAILED_VALUE);"
    },
    "  "
  );
  reused_region_canvas_repeated->add_variant_description_line("with repeated attempts");
  variants.push_back( reused_region_canvas_repeated );

  CodeCanvas code_array;
  code_array.add_global("char *target_addresses[16];");
  std::shared_ptr<StackRegion> stack_memory_region_array = std::make_shared<StackRegion>(*stack_memory_region);
  std::shared_ptr<RegionCodeCanvas> array_region_canvas = stack_memory_region_array->generate_array(std::make_shared<CodeCanvas>(code_array), "target", 8, 16, true);

  array_region_canvas->add_during_lifetime("for (int counter = 0; counter < 16; counter++) target_addresses[counter] = &target[counter][0];");
  access_type_code = access_location->generate_split_const_vars(
    access_action, "target_addresses[counter]", 8);
  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_simple_array = stack_memory_region_array->generate_in_other_f(
    array_region_canvas, "reallocated", 8, false
  );

  reused_region_canvas_simple_array->add_during_lifetime({
    "int counter;",
    "for (counter = 0; counter < 16; counter++)",
    "{",
    "  if ( GET_ADDR_BITS(&reallocated[0]) == GET_ADDR_BITS(target_addresses[counter]) ) break;",
    "}",
    "if (counter == 16) _exit(PRECONDITIONS_FAILED_VALUE);"
  });
  reused_region_canvas_simple_array->add_during_lifetime(access_type_code.access_lines);
  reused_region_canvas_simple_array->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
  reused_region_canvas_simple_array->add_globals( AccessLocation::AuxiliaryVariable::to_string_vector( access_type_code.aux_variables ) );
  reused_region_canvas_simple_array->add_variant_description_line("using an array of objects");
  variants.push_back( reused_region_canvas_simple_array );

  CodeCanvas code_array_repeated;
  code_array_repeated.add_global("char *target_addresses[16];");
  std::shared_ptr<StackRegion> stack_memory_region_array_repeated = std::make_shared<StackRegion>(*stack_memory_region);
  array_region_canvas = stack_memory_region_array_repeated->generate_array(std::make_shared<CodeCanvas>(code_array), "target", 8, 16, true);

  array_region_canvas->add_during_lifetime("for (int counter = 0; counter < 16; counter++) target_addresses[counter] = &target[counter][0];");
  access_type_code = access_location->generate_split_const_vars(
    access_action, "target_addresses[counter]", 8);
  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_array_repeated = stack_memory_region_array->generate_in_other_f(
    array_region_canvas, "reallocated", 8, false
  );
  reused_region_canvas_array_repeated->add_global("char *last_address = NULL;");
  reused_region_canvas_array_repeated->add_during_lifetime({
    "if ( GET_ADDR_BITS(last_address) == GET_ADDR_BITS(&reallocated[0]) ) _exit(PRECONDITIONS_FAILED_VALUE); // repeating does not help",
    "int counter;",
    "for (counter = 0; counter < 16; counter++)",
    "{",
    "  if ( GET_ADDR_BITS(&reallocated[0]) == GET_ADDR_BITS(target_addresses[counter]) ) break;",
    "}",
    "if (counter == 16) _exit(PRECONDITIONS_FAILED_VALUE);"
  });
  reused_region_canvas_array_repeated->add_during_lifetime(access_type_code.access_lines);
  reused_region_canvas_array_repeated->add_during_lifetime("return TEST_CASE_SUCCESSFUL_VALUE;");

  reused_region_canvas_array_repeated->add_globals( AccessLocation::AuxiliaryVariable::to_string_vector( access_type_code.aux_variables ) );

  reused_region_canvas_array_repeated->add_global("int counter = 0;");
  reused_region_canvas_array_repeated->prefix_line_with(reused_region_canvas_array_repeated->get_other_f_call_pos(), "    (void)");
  reused_region_canvas_array_repeated->add_at(reused_region_canvas_array_repeated->get_other_f_call_pos(),
    std::vector<std::string>{
      "do",
      "{"
    },
    "  "
  );
  reused_region_canvas_array_repeated->add_at(reused_region_canvas_array_repeated->get_other_f_call_pos() + 1,
    std::vector<std::string>{
      "} while (counter++ < " + max_reallocated_retries + ");",
      "_exit(PRECONDITIONS_FAILED_VALUE);"
    },
    "  "
    );
  reused_region_canvas_array_repeated->add_variant_description_line("with repeated attempts");
  reused_region_canvas_array_repeated->add_variant_description_line("using an array of objects");

  variants.push_back( reused_region_canvas_array_repeated );

  return variants;
}


std::vector< std::shared_ptr<RegionCodeCanvas> >UseAfterStar::generate_validation(
  std::shared_ptr<MemoryState> memory_state,
  std::shared_ptr<Region> memory_region,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location
  ) const
{
  if (is_a<FreedMemory>(memory_state))
  {
    return _generate_unused_mem_validation(memory_region, access_action, access_location);
  }
  assert(is_a<UsedMemory>(memory_state));
  return _generate_reused_mem_validation(memory_region, access_action, access_location);
}


std::vector< std::shared_ptr<RegionCodeCanvas> >UseAfterStar::_generate_unused_mem_validation(
  std::shared_ptr<Region> memory_region,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location) const
{
  if ( is_a<HeapRegion>(memory_region) ) return _generate_unused_mem_heap_validation(memory_region, access_action, access_location);
  assert( is_a<StackRegion>(memory_region) );
  return _generate_unused_mem_stack_validation(memory_region, access_action, access_location);
}

std::vector< std::shared_ptr<RegionCodeCanvas> >UseAfterStar::_generate_unused_mem_stack_validation(
  std::shared_ptr<Region> memory_region,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location) const
{

  std::vector< std::shared_ptr<RegionCodeCanvas> >variants = {};
  CodeCanvas code_simple;
  code_simple.add_global("char *target_address;");
  std::shared_ptr<StackRegion> stack_memory_region = std::dynamic_pointer_cast<StackRegion>(memory_region);
  std::shared_ptr<StackRegion> stack_memory_region_simple = std::make_shared<StackRegion>(*stack_memory_region);
  std::shared_ptr<RegionCodeCanvas> region_canvas = stack_memory_region_simple->generate(std::make_shared<CodeCanvas>(code_simple), "target", 8, true);

  region_canvas->add_to_f_body("target_address = &target[0];");
  AccessLocation::SplitAccess access_type_code = access_location->generate_split_const_vars(
    access_action, "target_address", 8);

  region_canvas->add_to_f_body(access_type_code.access_lines);
  region_canvas->add_to_f_body("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
  region_canvas->add_globals( AccessLocation::AuxiliaryVariable::to_string_vector( access_type_code.aux_variables ) );

  region_canvas->add_test_case_description_line("Memory region: stack");
  region_canvas->add_test_case_description_line("Bug type: use-after-*, freed memory");
  region_canvas->add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());


  variants.push_back( region_canvas );


  return variants;
}


std::vector< std::shared_ptr<RegionCodeCanvas> >UseAfterStar::_generate_unused_mem_heap_validation(
  std::shared_ptr<Region> memory_region,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location) const
{
  std::vector< std::shared_ptr<RegionCodeCanvas> >variants;
  CodeCanvas code;
  code.add_global("char *target_address;");
  std::shared_ptr<RegionCodeCanvas> region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(code), "target", 8, /*initialize=*/true);

  assert( is_a<HeapRegion>(memory_region) );

  std::vector<std::string> access_type_code = access_location->generate(
    access_action, "target_address", 8);
  auto index = region_canvas->add_to_f_body("target_address = &target[0];");
  index = region_canvas->add_at(index, access_type_code, "  ");
  region_canvas->add_at(index, "_exit(TEST_CASE_SUCCESSFUL_VALUE);", "  ");

  region_canvas->add_test_case_description_line("Memory region: heap");
  region_canvas->add_test_case_description_line("Bug type: use-after-*, freed memory");
  region_canvas->add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  variants.push_back( region_canvas );

  return variants;
}


std::vector< std::shared_ptr<RegionCodeCanvas> >UseAfterStar::_generate_reused_mem_validation(
  std::shared_ptr<Region> memory_region,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location) const
{
  if ( is_a<HeapRegion>(memory_region) ) return _generate_reused_mem_heap_validation(memory_region, access_action, access_location);
  assert( is_a<StackRegion>(memory_region) );
  return _generate_reused_mem_stack_validation(memory_region, access_action, access_location);
}

std::vector< std::shared_ptr<RegionCodeCanvas> >UseAfterStar::_generate_reused_mem_heap_validation(
  std::shared_ptr<Region> memory_region,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location) const
{
  std::vector< std::shared_ptr<RegionCodeCanvas> >variants;
  CodeCanvas code;
  code.add_global("char *target_address;");
  // code.add_global("char *last_address = NULL;");
  std::shared_ptr<RegionCodeCanvas> region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(code), "target", 8, false);

  assert ( is_a<HeapRegion>(memory_region) );

  region_canvas->add_during_lifetime("target_address = &target[0];");
    std::vector<std::string> access_type_code = access_location->generate(
    access_action, "target_address", 8);
  std::shared_ptr<HeapRegion> heap_memory_region = std::dynamic_pointer_cast<HeapRegion>(memory_region);

  std::shared_ptr<RegionCodeCanvas> reused_region_canvases = heap_memory_region->generate(
    region_canvas->get_lifetime_pos(), region_canvas, "reallocated", 8, false);
  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_simple = std::make_shared<RegionCodeCanvas>(*reused_region_canvases);

  reused_region_canvas_simple->add_during_lifetime(access_type_code);
  reused_region_canvas_simple->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");

  region_canvas->add_test_case_description_line("Memory region: heap");
  region_canvas->add_test_case_description_line("Bug type: use-after-*, reused memory");
  region_canvas->add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());


  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_repeat = std::make_shared<RegionCodeCanvas>(*reused_region_canvases);
  std::vector<std::string> allocation = heap_memory_region->generate_reallocation("reallocated", 8, true, "  ");
  std::vector<std::string> deallocation = heap_memory_region->generate_deallocation("reallocated", "  ");
  reused_region_canvas_repeat->add_during_lifetime({
    "size_t counter = 0;",
    "while ( counter < " + max_reallocated_retries_validation + ")",
    "{",
  });
  reused_region_canvas_repeat->add_during_lifetime(deallocation);
  reused_region_canvas_repeat->add_during_lifetime(allocation);
  reused_region_canvas_repeat->add_during_lifetime({
    "",
    "  counter++;",
    "}"
  });
  reused_region_canvas_repeat->add_during_lifetime(access_type_code);
  reused_region_canvas_repeat->add_during_lifetime("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
  reused_region_canvas_repeat->add_variant_description_line("with repeated attempts");
  variants = {reused_region_canvas_simple, reused_region_canvas_repeat};

  return variants;
}

std::vector< std::shared_ptr<RegionCodeCanvas> >UseAfterStar::_generate_reused_mem_stack_validation(
  std::shared_ptr<Region> memory_region,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location) const
{
  std::vector< std::shared_ptr<RegionCodeCanvas> >variants = {};
  CodeCanvas code_simple;
  code_simple.add_global("char *target_address;");
  code_simple.add_test_case_description_line("Memory region: heap");
  code_simple.add_test_case_description_line("Bug type: use-after-*, reused memory");
  code_simple.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  std::shared_ptr<StackRegion> stack_memory_region = std::dynamic_pointer_cast<StackRegion>(memory_region);
  std::shared_ptr<StackRegion> stack_memory_region_simple = std::make_shared<StackRegion>(*stack_memory_region);
  std::shared_ptr<RegionCodeCanvas> region_canvas = stack_memory_region_simple->generate(std::make_shared<CodeCanvas>(code_simple), "target", 8, true);

  region_canvas->add_during_lifetime("target_address = &target[0];");
  AccessLocation::SplitAccess access_type_code = access_location->generate_split_const_vars(
    access_action, "target_address", 8);

  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_simple = stack_memory_region_simple->generate(
    region_canvas, "reallocated", 8, false
  );
  reused_region_canvas_simple->add_to_f_body(access_type_code.access_lines);
  reused_region_canvas_simple->add_to_f_body("_use(reallocated);");
  reused_region_canvas_simple->add_to_f_body("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
  reused_region_canvas_simple->add_globals( AccessLocation::AuxiliaryVariable::to_string_vector( access_type_code.aux_variables ) );
  variants.push_back( reused_region_canvas_simple );

  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_repeated = stack_memory_region_simple->generate(
    region_canvas, "reallocated", 8, false
    );
  reused_region_canvas_repeated->add_global("int counter = 0;");
  reused_region_canvas_repeated->add_global("char *last_address = NULL;");
  reused_region_canvas_repeated->add_to_f_body(
    "last_address = &reallocated[0];"
  );
  reused_region_canvas_repeated->add_to_f_body(access_type_code.access_lines);
  reused_region_canvas_repeated->add_globals( AccessLocation::AuxiliaryVariable::to_string_vector( access_type_code.aux_variables ) );
  reused_region_canvas_repeated->prefix_line_with(reused_region_canvas_repeated->get_f_call_pos(), "    (void)");
  reused_region_canvas_repeated->add_at(reused_region_canvas_repeated->get_f_call_pos(),
    std::vector<std::string>{
      "do",
      "{"
    },
    "  "
  );
  reused_region_canvas_repeated->add_at(reused_region_canvas_repeated->get_f_call_pos() + 1,
    std::vector<std::string>{
      "} while (counter++ < " + max_reallocated_retries_validation + ");",
      "_exit(TEST_CASE_SUCCESSFUL_VALUE);"
    },
    "  "
  );
  reused_region_canvas_repeated->add_variant_description_line("with repeated attempts");

  variants.push_back( reused_region_canvas_repeated );

  CodeCanvas code_array;
  code_array.add_global("char *target_addresses[16];");
  std::shared_ptr<StackRegion> stack_memory_region_array = std::make_shared<StackRegion>(*stack_memory_region);
  std::shared_ptr<RegionCodeCanvas> array_region_canvas = stack_memory_region_array->generate_array(std::make_shared<CodeCanvas>(code_array), "target", 8, 16, true);

  array_region_canvas->add_during_lifetime("for (int counter = 0; counter < 16; counter++) target_addresses[counter] = &target[counter][0];");
  access_type_code = access_location->generate_split_const_vars(
    access_action, "target_addresses[counter]", 8);
  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_simple_array = stack_memory_region_array->generate(
    array_region_canvas, "reallocated", 8, false
  );

  reused_region_canvas_simple_array->add_to_f_body({
    "int counter;",
    "for (counter = 0; counter < 16; counter++)",
    "{",
    "  if ( GET_ADDR_BITS(&reallocated[0]) == GET_ADDR_BITS(target_addresses[counter]) ) break;",
    "}",
    "if (counter == 16) counter = 0;"
  });
  reused_region_canvas_simple_array->add_to_f_body(access_type_code.access_lines);
  reused_region_canvas_simple_array->add_to_f_body("_exit(TEST_CASE_SUCCESSFUL_VALUE);");
  reused_region_canvas_simple_array->add_globals( AccessLocation::AuxiliaryVariable::to_string_vector( access_type_code.aux_variables ) );
  reused_region_canvas_simple_array->add_variant_description_line("using an array of objects");

  variants.push_back( reused_region_canvas_simple_array );

  CodeCanvas code_array_repeated;
  code_array_repeated.add_global("char *target_addresses[16];");
  std::shared_ptr<StackRegion> stack_memory_region_array_repeated = std::make_shared<StackRegion>(*stack_memory_region);
  array_region_canvas = stack_memory_region_array_repeated->generate_array(std::make_shared<CodeCanvas>(code_array), "target", 8, 16, true);

  array_region_canvas->add_to_f_body("for (int counter = 0; counter < 16; counter++) target_addresses[counter] = &target[counter][0];");
  access_type_code = access_location->generate_split_const_vars(
    access_action, "target_addresses[counter]", 8);
  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_array_repeated = stack_memory_region_array->generate(
    array_region_canvas, "reallocated", 8, false
  );
  reused_region_canvas_array_repeated->add_global("char *last_address = NULL;");
  reused_region_canvas_array_repeated->add_to_f_body({
    "int counter;",
    "for (counter = 0; counter < 16; counter++)",
    "{",
    "  if ( GET_ADDR_BITS(&reallocated[0]) == GET_ADDR_BITS(target_addresses[counter]) ) break;",
    "}",
    "if (counter == 16) counter = 0;"
  });
  reused_region_canvas_array_repeated->add_to_f_body(access_type_code.access_lines);
  reused_region_canvas_array_repeated->add_to_f_body("return TEST_CASE_SUCCESSFUL_VALUE;");
  reused_region_canvas_array_repeated->add_globals( AccessLocation::AuxiliaryVariable::to_string_vector( access_type_code.aux_variables ) );

  reused_region_canvas_array_repeated->add_global("int counter = 0;");
  reused_region_canvas_array_repeated->prefix_line_with(reused_region_canvas_array_repeated->get_f_call_pos(), "    (void)");
  reused_region_canvas_array_repeated->add_at(reused_region_canvas_array_repeated->get_f_call_pos(),
    std::vector<std::string>{
      "do",
      "{"
    },
    "  "
  );
  reused_region_canvas_array_repeated->add_at(reused_region_canvas_array_repeated->get_f_call_pos() + 1,
    std::vector<std::string>{
      "} while (counter++ < " + max_reallocated_retries_validation + ");",
      "_exit(TEST_CASE_SUCCESSFUL_VALUE);"
    },
    "  "
  );
  reused_region_canvas_array_repeated->add_variant_description_line("repeated attempts");
  reused_region_canvas_array_repeated->add_variant_description_line("using an array of objects");

  variants.push_back( reused_region_canvas_array_repeated );

  return variants;
}
