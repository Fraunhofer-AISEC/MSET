/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "double_free.h"

#include "generator/primitives/access_types/access_action.h"
#include "generator/primitives/access_types/access_location.h"

#include "misc.h"
#include "generator/primitives/bug_types/temporal/memory_state/used.h"
#include "generator/primitives/regions/heap_region.h"

DoubleFree::DoubleFree():
  TemporalBugType("double_free")
{
}

bool DoubleFree::accepts(std::shared_ptr<MemoryState> memory_state)
{
  return is_a<UsedMemory>(memory_state);
}

bool DoubleFree::accepts(std::shared_ptr<Region> region)
{
  return is_a<HeapRegion>(region);
}

bool DoubleFree::accepts(std::shared_ptr<AccessLocation> access_type)
{
  return true;
}

std::vector< std::shared_ptr<RegionCodeCanvas> >DoubleFree::generate(
  std::shared_ptr<MemoryState> memory_state,
  std::shared_ptr<Region> memory_region,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location
  ) const
{
  std::vector< std::shared_ptr<RegionCodeCanvas> >full_variants;
  /*
    #ifndef __GLIBC__
    exit(PRECONDITIONS_FAILED_VALUE); // not using glibc
    #endif
    char *pointer_to_double_free; // pointer to be double-freed
    char *pointer_to_use; // pointer to illegally use
    pointer_to_double_free = (char *)malloc(10);
    free(pointer_to_double_free);
    pointer_to_double_free[sizeof(void *)] = 0; // use-after-free for heap metadata corruption
    free(pointer_to_double_free); // double free
    pointer_to_use = (char *)malloc(8); // allocate a new object
    <target_allocation>
    <action>
    _exit(42);
    <target_deallocation>
  */
  CodeCanvas variant_with_use_after_free;

  variant_with_use_after_free.add_test_case_description_line("Memory region: " + memory_region->get_name());
  variant_with_use_after_free.add_test_case_description_line("Bug type: double-free, " + memory_state->get_printable_name());
  variant_with_use_after_free.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  variant_with_use_after_free.add_to_variant_description("with use-after-free");

  variant_with_use_after_free.add_to_f_body({
    "#ifndef __GLIBC__",
    "exit(PRECONDITIONS_FAILED_VALUE); // not using glibc",
    "#endif",
    "char *pointer_to_double_free; // pointer to be double-freed",
    "char *pointer_to_use; // pointer to illegally use",
    "pointer_to_double_free = (char *)malloc(10);",
    "free(pointer_to_double_free);",
    "pointer_to_double_free[sizeof(void *)] = 0; // use-after-free for heap metadata corruption",
    "free(pointer_to_double_free); // double free",
    "pointer_to_use = (char *)malloc(8); // allocate a new object"
  });

  std::shared_ptr<RegionCodeCanvas> region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(variant_with_use_after_free), "target", 8, false);


  std::vector<std::string> access_type_code = access_location->generate(
    access_action, "pointer_to_use",  region_canvas->get_static_var_size());
  auto index = region_canvas->add_at(region_canvas->get_lifetime_pos(), access_type_code, "  ");
  region_canvas->add_at(index, "  _exit(TEST_CASE_SUCCESSFUL_VALUE);");
  full_variants.push_back( region_canvas );

  CodeCanvas variant_without_use_after_free;
  variant_without_use_after_free.add_test_case_description_line("Memory region: " + memory_region->get_name());
  variant_without_use_after_free.add_test_case_description_line("Bug type: double-free, " + memory_state->get_printable_name());
  variant_without_use_after_free.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  variant_without_use_after_free.add_to_variant_description("without use-after-free");
  variant_without_use_after_free.add_to_f_body({
    "#ifndef __GLIBC__",
    "exit(PRECONDITIONS_FAILED_VALUE); // not using glibc",
    "#endif",
    "char *pointer_to_double_free; // pointer to be double-freed",
    "char *pointer_to_use; // pointer to illegally use",
    "char *tmp, *tmp2, *tmp3;",
    "tmp = (char *)malloc(8);",
    "tmp2 = (char *)malloc(8);",
    "pointer_to_double_free = (char *)malloc(8);",
    "free(pointer_to_double_free);",
    "free(tmp); // no use after free required",
    "free(pointer_to_double_free); // double free",
    "pointer_to_use = (char *)malloc(8); // allocate a new object",
    "tmp3 = (char *)malloc(8);",
    "_use(tmp2);",
    "_use(tmp3);"
  });

  region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(variant_without_use_after_free), "target", 8, false);

  access_type_code = access_location->generate(
    access_action, "pointer_to_use", region_canvas->get_static_var_size());
  index = region_canvas->add_at(region_canvas->get_lifetime_pos(), access_type_code, "  ");
  region_canvas->add_at(index, "  _exit(TEST_CASE_SUCCESSFUL_VALUE);");
  full_variants.push_back( region_canvas );

  return full_variants;
}


std::vector< std::shared_ptr<RegionCodeCanvas> >DoubleFree::generate_validation(
  std::shared_ptr<MemoryState> memory_state,
  std::shared_ptr<Region> memory_region,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location
  ) const
{
  std::vector< std::shared_ptr<RegionCodeCanvas> >full_variants;
  /*
    #ifndef __GLIBC__
    exit(PRECONDITIONS_FAILED_VALUE); // not using glibc
    #endif
    char *pointer_to_double_free; // pointer to be double-freed
    char *pointer_to_use; // pointer to illegally use
    pointer_to_double_free = (char *)malloc(10);
    free(pointer_to_double_free);
    pointer_to_use = (char *)malloc(8); // allocate a new object
    <target_allocation>
    <action>
    _exit(42);
    <target_deallocation>
  */
  CodeCanvas variant_with_use_after_free;

  variant_with_use_after_free.add_test_case_description_line("Memory region: " + memory_region->get_name());
  variant_with_use_after_free.add_test_case_description_line("Bug type: double-free, " + memory_state->get_printable_name());
  variant_with_use_after_free.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  variant_with_use_after_free.add_to_variant_description("with use-after-free");

  variant_with_use_after_free.add_to_f_body({
    "#ifndef __GLIBC__",
    "exit(PRECONDITIONS_FAILED_VALUE); // not using glibc",
    "#endif",
    "char *pointer_to_double_free; // pointer to be double-freed",
    "char *pointer_to_use; // pointer to illegally use",
    "pointer_to_double_free = (char *)malloc(10);",
    "free(pointer_to_double_free);",
    "pointer_to_use = (char *)malloc(8); // allocate a new object"
  });

  std::shared_ptr<RegionCodeCanvas> region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(variant_with_use_after_free), "target", 8, false);

  std::vector<std::string> access_type_code = access_location->generate(
    access_action, "pointer_to_use",  region_canvas->get_static_var_size());
  auto index = region_canvas->add_at(region_canvas->get_lifetime_pos(), access_type_code, "  ");
  region_canvas->add_at(index, "  _exit(TEST_CASE_SUCCESSFUL_VALUE);");
  full_variants.push_back( region_canvas );

  CodeCanvas variant_without_use_after_free;

  variant_without_use_after_free.add_test_case_description_line("Memory region: " + memory_region->get_name());
  variant_without_use_after_free.add_test_case_description_line("Bug type: double-free, " + memory_state->get_printable_name());
  variant_without_use_after_free.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  variant_without_use_after_free.add_to_variant_description("without use-after-free");

  variant_without_use_after_free.add_to_f_body({
    "#ifndef __GLIBC__",
    "exit(PRECONDITIONS_FAILED_VALUE); // not using glibc",
    "#endif",
    "char *pointer_to_double_free; // pointer to be double-freed",
    "char *pointer_to_use; // pointer to illegally use",
    "char *tmp, *tmp2, *tmp3;",
    "tmp = (char *)malloc(8);",
    "tmp2 = (char *)malloc(8);",
    "pointer_to_double_free = (char *)malloc(8);",
    "free(pointer_to_double_free);",
    "free(tmp); // no use after free required",
    "pointer_to_use = (char *)malloc(8); // allocate a new object",
    "tmp3 = (char *)malloc(8);",
    "_use(tmp2);",
    "_use(tmp3);"
  });

  region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(variant_without_use_after_free), "target", 8, false);

  access_type_code = access_location->generate(
    access_action, "pointer_to_use", region_canvas->get_static_var_size());
  index = region_canvas->add_at(region_canvas->get_lifetime_pos(), access_type_code, "  ");
  region_canvas->add_at(index, "  _exit(TEST_CASE_SUCCESSFUL_VALUE);");
  full_variants.push_back( region_canvas );

  return full_variants;
}
