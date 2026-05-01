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
    %pointer_to_double_free = memref.alloc() : memref<10xi8>
    %pointer_to_use = memref.alloc() : memref<8xi8>
    memref.dealloc %pointer_to_double_free : memref<10xi8>
    memref.store %c0_i8, %pointer_to_double_free[%c8] : memref<10xi8>
    memref.dealloc %pointer_to_double_free : memref<10xi8>
    <target_allocation>
    <action>
    func.return %test_success : i32
    <target_deallocation>
  */
  CodeCanvas variant_with_use_after_free;

  variant_with_use_after_free.add_test_case_description_line("Memory region: " + memory_region->get_name());
  variant_with_use_after_free.add_test_case_description_line("Bug type: double-free, " + memory_state->get_printable_name());
  variant_with_use_after_free.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  variant_with_use_after_free.add_variant_description_line("with use-after-free");

  variant_with_use_after_free.add_global("func.func private @exit(%arg0: i32)  -> ()");
  variant_with_use_after_free.add_global("func.func @use(%arg0: memref<8xi8>) -> memref<8xi8> { return %arg0 : memref<8xi8> }");
  variant_with_use_after_free.add_to_f_body({
    "%c8_df = arith.constant 8 : index",
    "%c0_i8_df = arith.constant 0 : i8",
    "%pointer_to_double_free = memref.alloc() : memref<10xi8> // pointer to be double-freed",
    "memref.dealloc %pointer_to_double_free : memref<10xi8>",
    "memref.store %c0_i8_df, %pointer_to_double_free[%c8_df] : memref<10xi8> // use-after-free for heap metadata corruption",
    "memref.dealloc %pointer_to_double_free : memref<10xi8> // double free",
    "%pointer_to_use = memref.alloc() : memref<8xi8> // allocate a new object"
  });

  std::shared_ptr<RegionCodeCanvas> region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(variant_with_use_after_free), "target", 8, false);


  std::vector<std::string> access_type_code = access_location->generate(
    access_action, "pointer_to_use",  region_canvas->get_static_var_size());
  auto index = region_canvas->add_at(region_canvas->get_lifetime_pos(), access_type_code, "  ");
  region_canvas->add_at(index, "  func.call @exit(%test_success) : (i32) -> ()");
  full_variants.push_back( region_canvas );

  CodeCanvas variant_without_use_after_free;
  variant_without_use_after_free.add_test_case_description_line("Memory region: " + memory_region->get_name());
  variant_without_use_after_free.add_test_case_description_line("Bug type: double-free, " + memory_state->get_printable_name());
  variant_without_use_after_free.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  variant_without_use_after_free.add_variant_description_line("without use-after-free");
  variant_without_use_after_free.add_global("func.func private @exit(%arg0: i32) -> ()");
  variant_without_use_after_free.add_global("func.func @use(%arg0: memref<8xi8>) -> memref<8xi8> { return %arg0 : memref<8xi8> }");
  variant_without_use_after_free.add_to_f_body({
    "%tmp = memref.alloc() : memref<8xi8>",
    "%tmp2 = memref.alloc() : memref<8xi8>",
    "%pointer_to_double_free = memref.alloc() : memref<8xi8> // pointer to be double-freed",
    "memref.dealloc %pointer_to_double_free : memref<8xi8>",
    "memref.dealloc %tmp : memref<8xi8> // no use after free required",
    "memref.dealloc %pointer_to_double_free : memref<8xi8> // double free",
    "%pointer_to_use = memref.alloc() : memref<8xi8> // allocate a new object",
    "%tmp3 = memref.alloc() : memref<8xi8>"
    // "func.call @use(%tmp2) : (memref<8xi8>) -> ()",
    // "func.call @use(%tmp3) : (memref<8xi8>) -> ()"
  });

  region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(variant_without_use_after_free), "target", 8, false);

  access_type_code = access_location->generate(
    access_action, "pointer_to_use", region_canvas->get_static_var_size());
  index = region_canvas->add_at(region_canvas->get_lifetime_pos(), access_type_code, "  ");
  region_canvas->add_at(index, "  func.call @exit(%test_success) : (i32) -> ()");
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
    %pointer_to_double_free = memref.alloc() : memref<10xi8>
    %pointer_to_use = memref.alloc() : memref<8xi8>
    memref.dealloc %pointer_to_double_free : memref<10xi8>
    <target_allocation>
    <action>
    func.return %test_success : i32
    <target_deallocation>
  */
  CodeCanvas variant_with_use_after_free;

  variant_with_use_after_free.add_test_case_description_line("Memory region: " + memory_region->get_name());
  variant_with_use_after_free.add_test_case_description_line("Bug type: double-free, " + memory_state->get_printable_name());
  variant_with_use_after_free.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  variant_with_use_after_free.add_variant_description_line("with use-after-free");

  variant_with_use_after_free.add_global("func.func private @exit(%arg0: i32) -> ()");
  variant_with_use_after_free.add_global("func.func @use(%arg0: memref<8xi8>) -> memref<8xi8> { return %arg0 : memref<8xi8> }");
  variant_with_use_after_free.add_to_f_body({
    "%pointer_to_double_free = memref.alloc() : memref<10xi8> // pointer to be double-freed",
    "memref.dealloc %pointer_to_double_free : memref<10xi8>",
    "%pointer_to_use = memref.alloc() : memref<8xi8> // allocate a new object"
  });

  std::shared_ptr<RegionCodeCanvas> region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(variant_with_use_after_free), "target", 8, false);

  std::vector<std::string> access_type_code = access_location->generate(
    access_action, "pointer_to_use",  region_canvas->get_static_var_size());
  auto index = region_canvas->add_at(region_canvas->get_lifetime_pos(), access_type_code, "  ");
  region_canvas->add_at(index, "  func.call @exit(%test_success) : (i32) -> ()");
  full_variants.push_back( region_canvas );

  CodeCanvas variant_without_use_after_free;

  variant_without_use_after_free.add_test_case_description_line("Memory region: " + memory_region->get_name());
  variant_without_use_after_free.add_test_case_description_line("Bug type: double-free, " + memory_state->get_printable_name());
  variant_without_use_after_free.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  variant_without_use_after_free.add_variant_description_line("without use-after-free");
  variant_without_use_after_free.add_global("func.func private @exit(%arg0: i32) -> ()");
  variant_without_use_after_free.add_global("func.func @use(%arg0: memref<8xi8>) -> memref<8xi8> { return %arg0 : memref<8xi8> }");

  variant_without_use_after_free.add_to_f_body({
    "%tmp = memref.alloc() : memref<8xi8>",
    "%tmp2 = memref.alloc() : memref<8xi8>",
    "%pointer_to_double_free = memref.alloc() : memref<8xi8> // pointer to be double-freed",
    "memref.dealloc %pointer_to_double_free : memref<8xi8>",
    "memref.dealloc %tmp : memref<8xi8> // no use after free required",
    "%pointer_to_use = memref.alloc() : memref<8xi8> // allocate a new object",
    "%tmp3 = memref.alloc() : memref<8xi8>"
    // "func.call @use(%tmp2) : (memref<8xi8>) -> ()",
    // "func.call @use(%tmp3) : (memref<8xi8>) -> ()"
  });

  region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(variant_without_use_after_free), "target", 8, false);

  access_type_code = access_location->generate(
    access_action, "pointer_to_use", region_canvas->get_static_var_size());
  index = region_canvas->add_at(region_canvas->get_lifetime_pos(), access_type_code, "  ");
  region_canvas->add_at(index, "  func.call @exit(%test_success) : (i32) -> ()");
  full_variants.push_back( region_canvas );

  return full_variants;
}
