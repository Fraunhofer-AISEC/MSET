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

static std::vector<std::string> get_filtered_aux_lines(
    const AccessLocation::SplitAccess &access)
{
    std::vector<std::string> result;
    for (const auto &var : access.aux_variables) {
        if (var.name == "c0" || var.name == "c1" || var.name == "c8")
            continue;
        result.push_back(var.to_string());
    }
    return result;
}

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
  code_simple.add_global("memref.global @target_address : memref<1xindex>");
  code_simple.add_global("memref.global @target_ptr : memref<1xmemref<8xi8>>");
  code_simple.add_global("func.func private @exit(%arg0: i32) -> ()");
  std::shared_ptr<StackRegion> stack_memory_region = std::dynamic_pointer_cast<StackRegion>(memory_region);
  std::shared_ptr<StackRegion> stack_memory_region_simple = std::make_shared<StackRegion>(*stack_memory_region);
  std::shared_ptr<RegionCodeCanvas> region_canvas = stack_memory_region_simple->generate(std::make_shared<CodeCanvas>(code_simple), "target", 8, true);

  region_canvas->add_during_lifetime({
    "%target_addr = memref.extract_aligned_pointer_as_index %target : memref<8xi8> -> index",
    "%global_addr = memref.get_global @target_address : memref<1xindex>",
    "memref.store %target_addr, %global_addr[%c0] : memref<1xindex>",
    "%global_ptr = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "memref.store %target, %global_ptr[%c0] : memref<1xmemref<8xi8>>"
  });
  AccessLocation::SplitAccess access_type_code = access_location->generate_split_const_vars(
    access_action, "saved_ptr", 8);

  std::vector<std::string> lines = {
    "%c0 = arith.constant 0 : index",
    "%global_ptr_main = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "%saved_ptr = memref.load %global_ptr_main[%c0] : memref<1xmemref<8xi8>>"
  };
  region_canvas->add_to_main_body(lines);
  region_canvas->add_to_main_body(access_type_code.to_lines());
  region_canvas->add_to_main_body("%test_success = arith.constant 42 : i32");
  region_canvas->add_to_main_body("func.call @exit(%test_success) : (i32) -> ()");

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
  code.add_global("memref.global @target_address : memref<1xindex>");
  code.add_global("memref.global @target_ptr : memref<1xmemref<8xi8>>");
  code.add_global("func.func private @exit(%arg0: i32) -> ()");

  std::shared_ptr<RegionCodeCanvas> region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(code), "target", 8, /*initialize=*/true);

  assert( is_a<HeapRegion>(memory_region) );

  // Save target address and ptr to globals AFTER deallocation
  auto index = region_canvas->add_at(region_canvas->get_deallocation_pos(), {
    "%target_addr = memref.extract_aligned_pointer_as_index %target : memref<8xi8> -> index",
    "%global_addr = memref.get_global @target_address : memref<1xindex>",
    "memref.store %target_addr, %global_addr[%c0] : memref<1xindex>",
    "%global_ptr = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "memref.store %target, %global_ptr[%c0] : memref<1xmemref<8xi8>>"
  }, "  ");

  // Load saved ptr and access
  std::vector<std::string> access_type_code = access_location->generate(
    access_action, "saved_ptr", 8);
  std::vector<std::string> lines = {
    "%global_ptr_main = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "%saved_ptr = memref.load %global_ptr_main[%c0] : memref<1xmemref<8xi8>>"
  };
  index = region_canvas->add_at(index, lines, "  ");
  index = region_canvas->add_at(index, access_type_code, "  ");
  region_canvas->add_at(index, "func.call @exit(%test_success) : (i32) -> ()", "  ");
  // region_canvas->add_at(index, "func.return %test_success : i32", "  ");

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
  code.add_global("memref.global @target_address : memref<1xindex>");
  code.add_global("memref.global @target_ptr : memref<1xmemref<8xi8>>");
  code.add_global("func.func private @exit(%arg0: i32) -> ()");

  std::shared_ptr<RegionCodeCanvas> region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(code), "target", 8, true);

  assert ( is_a<HeapRegion>(memory_region) );

  region_canvas->add_during_lifetime({
    "%target_addr = memref.extract_aligned_pointer_as_index %target : memref<8xi8> -> index",
    "%global_addr = memref.get_global @target_address : memref<1xindex>",
    "memref.store %target_addr, %global_addr[%c0] : memref<1xindex>",
    "%global_ptr = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "memref.store %target, %global_ptr[%c0] : memref<1xmemref<8xi8>>"
  });

  std::shared_ptr<HeapRegion> heap_memory_region = std::dynamic_pointer_cast<HeapRegion>(memory_region);

  std::shared_ptr<RegionCodeCanvas> reused_region_canvas = heap_memory_region->generate(
    region_canvas->get_deallocation_pos(), region_canvas, "reallocated_out", 8, false);
  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_simple = std::make_shared<RegionCodeCanvas>(*reused_region_canvas);

  std::vector<std::string> access_type_code = access_location->generate(
    access_action, "saved_ptr", 8);

  reused_region_canvas_simple->add_during_lifetime({
    "%realloc_addr = memref.extract_aligned_pointer_as_index %reallocated_out : memref<8xi8> -> index",
    "%eq = arith.cmpi eq, %target_addr, %realloc_addr : index",
    "%result = scf.if %eq -> i32 {",
    "  %global_ptr_access = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "  %saved_ptr = memref.load %global_ptr_access[%c0] : memref<1xmemref<8xi8>>"
  });
  reused_region_canvas_simple->add_during_lifetime(access_type_code);
  reused_region_canvas_simple->add_during_lifetime({
    "  scf.yield %test_success : i32",
    "} else {",
    "  scf.yield %precond_fail : i32",
    "}",
    "func.call @exit(%result) : (i32) -> ()"
  });

  region_canvas->add_test_case_description_line("Memory region: heap");
  region_canvas->add_test_case_description_line("Bug type: use-after-*, reused memory");
  region_canvas->add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_repeat = std::make_shared<RegionCodeCanvas>(*reused_region_canvas);
  std::vector<std::string> allocation = heap_memory_region->generate_reallocation("reallocated", 8, true, "    ");
  std::vector<std::string> deallocation = heap_memory_region->generate_deallocation("reallocated", 8, "    ");

  reused_region_canvas_repeat->add_during_lifetime({
    "memref.dealloc %reallocated_out : memref<8xi8>",
    "%c0 = arith.constant 0 : index",
    "%c1 = arith.constant 1 : index",
    "%cMAX = arith.constant " + max_reallocated_retries + " : index",
    "%false = arith.constant false",
    "%counter_final, %matched_final = scf.while (%counter = %c0, %matched = %false) : (index, i1) -> (index, i1) {",
    "  %lt = arith.cmpi slt, %counter, %cMAX : index",
    "  scf.condition(%lt) %counter, %matched : index, i1",
    "} do {",
    "  ^bb0(%counter_iter : index, %matched_iter : i1):"
  });
  reused_region_canvas_repeat->add_during_lifetime(allocation);
  reused_region_canvas_repeat->add_during_lifetime({
    "    %realloc_addr = memref.extract_aligned_pointer_as_index %reallocated : memref<8xi8> -> index",
    "    %eq = arith.cmpi eq, %target_addr, %realloc_addr : index",
    "    %next_matched = arith.ori %matched_iter, %eq : i1"
  });
  reused_region_canvas_repeat->add_during_lifetime(deallocation);
  reused_region_canvas_repeat->add_during_lifetime({
    "    %next_counter = arith.addi %counter_iter, %c1 : index",
    "    scf.yield %next_counter, %next_matched : index, i1",
    "}",
    "%result = scf.if %matched_final -> i32 {",
    "  %global_ptr_access = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "  %saved_ptr = memref.load %global_ptr_access[%c0] : memref<1xmemref<8xi8>>"
  });
  reused_region_canvas_repeat->add_during_lifetime(access_type_code);
  reused_region_canvas_repeat->add_during_lifetime({
    "  scf.yield %test_success : i32",
    "} else {",
    "  scf.yield %precond_fail : i32",
    "}",
    "func.call @exit(%result) : (i32) -> ()"
  });

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
  code_simple.add_global("memref.global @target_address : memref<1xindex>");
  code_simple.add_global("memref.global @target_ptr : memref<1xmemref<8xi8>>");
  code_simple.add_global("func.func private @exit(%arg0: i32) -> ()");
  std::shared_ptr<StackRegion> stack_memory_region = std::dynamic_pointer_cast<StackRegion>(memory_region);
  std::shared_ptr<StackRegion> stack_memory_region_simple = std::make_shared<StackRegion>(*stack_memory_region);
  std::shared_ptr<RegionCodeCanvas> region_canvas = stack_memory_region_simple->generate(std::make_shared<CodeCanvas>(code_simple), "target", 8, true);

  region_canvas->add_during_lifetime({
    "%target_addr = memref.extract_aligned_pointer_as_index %target : memref<8xi8> -> index",
    "%global_addr = memref.get_global @target_address : memref<1xindex>",
    "memref.store %target_addr, %global_addr[%c0] : memref<1xindex>",
    "%global_ptr = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "memref.store %target, %global_ptr[%c0] : memref<1xmemref<8xi8>>"
  });
  AccessLocation::SplitAccess access_type_code = access_location->generate_split_const_vars(
    access_action, "saved_ptr", 8);

  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_simple = stack_memory_region_simple->generate_in_other_f(
    region_canvas, "reallocated", 8, false
  );
  reused_region_canvas_simple->add_during_lifetime({
    "%precond_fail = arith.constant 43 : i32",
    "%test_success = arith.constant 42 : i32",
    "%c0 = arith.constant 0 : index",
    "%realloc_addr = memref.extract_aligned_pointer_as_index %reallocated : memref<8xi8> -> index",
    "%global_addr_other = memref.get_global @target_address : memref<1xindex>",
    "%target_addr_loaded = memref.load %global_addr_other[%c0] : memref<1xindex>",
    "%eq = arith.cmpi eq, %target_addr_loaded, %realloc_addr : index",
    "scf.if %eq {",
    "  %global_ptr_access = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "  %saved_ptr = memref.load %global_ptr_access[%c0] : memref<1xmemref<8xi8>>"
  });
  reused_region_canvas_simple->add_during_lifetime(access_type_code.to_lines());
  reused_region_canvas_simple->add_during_lifetime({
    "  func.call @exit(%test_success) : (i32) -> ()",
    "  scf.yield",
    "}",
    "func.call @exit(%precond_fail) : (i32) -> ()"
  });

  region_canvas->add_test_case_description_line("Memory region: stack");
  region_canvas->add_test_case_description_line("Bug type: use-after-*, reused memory");
  region_canvas->add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  variants.push_back( reused_region_canvas_simple );

  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_repeated = stack_memory_region_simple->generate_in_other_f(
    region_canvas, "reallocated", 8, false
    );
  reused_region_canvas_repeated->add_global("memref.global @last_address : memref<1xindex>");

  AccessLocation::SplitAccess access_type_code_repeat = access_location->generate_split_const_vars(
    access_action, "saved_ptr", 8);

  reused_region_canvas_repeated->add_during_lifetime({
    "%precond_fail = arith.constant 43 : i32",
    "%test_success = arith.constant 42 : i32",
    "%c0 = arith.constant 0 : index",
    "%realloc_addr = memref.extract_aligned_pointer_as_index %reallocated : memref<8xi8> -> index",
    "%global_last = memref.get_global @last_address : memref<1xindex>",
    "%last_addr = memref.load %global_last[%c0] : memref<1xindex>",
    "%eq_last = arith.cmpi eq, %last_addr, %realloc_addr : index",
    "scf.if %eq_last {",
    "  func.call @exit(%precond_fail) : (i32) -> ()",
    "  scf.yield",
    "}",
    "memref.store %realloc_addr, %global_last[%c0] : memref<1xindex>",
    "%global_addr_other = memref.get_global @target_address : memref<1xindex>",
    "%target_addr_loaded = memref.load %global_addr_other[%c0] : memref<1xindex>",
    "%eq = arith.cmpi eq, %target_addr_loaded, %realloc_addr : index",
    "scf.if %eq {",
    "  %global_ptr_access = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "  %saved_ptr = memref.load %global_ptr_access[%c0] : memref<1xmemref<8xi8>>"
  });
  reused_region_canvas_repeated->add_during_lifetime(access_type_code_repeat.to_lines());
  reused_region_canvas_repeated->add_during_lifetime({
    "  func.call @exit(%test_success) : (i32) -> ()",
    "  scf.yield",
    "}"
  });

  reused_region_canvas_repeated->add_at(reused_region_canvas_repeated->get_other_f_call_pos(),
    std::vector<std::string>{
      "%c0_main = arith.constant 0 : index",
      "%c1_main = arith.constant 1 : index",
      "%cMAX_main = arith.constant " + max_reallocated_retries + " : index",
      "%results = scf.while (%counter = %c0_main) : (index) -> index {",
      "  %lt = arith.cmpi slt, %counter, %cMAX_main : index",
      "  scf.condition(%lt) %counter : index",
      "} do {",
      "^bb0(%counter_iter : index):"
    },
    "    "
  );
  reused_region_canvas_repeated->add_at(reused_region_canvas_repeated->get_other_f_call_pos() + 1,
    std::vector<std::string>{
      "  %next_counter = arith.addi %counter_iter, %c1_main : index",
      "  scf.yield %next_counter : index",
      "}",
      "%precond_fail = arith.constant 43 : i32",
      "func.call @exit(%precond_fail) : (i32) -> ()"
    },
    "    "
  );
  reused_region_canvas_repeated->add_variant_description_line("with repeated attempts");
  variants.push_back( reused_region_canvas_repeated );

  CodeCanvas code_array;
  code_array.add_global("memref.global @target_addresses : memref<16xindex>");
  code_array.add_global("memref.global @target_arr : memref<1xmemref<16x8xi8>>");
  code_array.add_global("func.func private @exit(%arg0: i32) -> ()");
  std::shared_ptr<StackRegion> stack_memory_region_array = std::make_shared<StackRegion>(*stack_memory_region);
  std::shared_ptr<RegionCodeCanvas> array_region_canvas = stack_memory_region_array->generate_array(std::make_shared<CodeCanvas>(code_array), "target", 8, 16, true);

  array_region_canvas->add_during_lifetime({
    "%global_arr = memref.get_global @target_arr : memref<1xmemref<16x8xi8>>",
    "memref.store %target, %global_arr[%c0] : memref<1xmemref<16x8xi8>>",
    "%base_addr = memref.extract_aligned_pointer_as_index %target : memref<16x8xi8> -> index",
    "%global_addrs = memref.get_global @target_addresses : memref<16xindex>",
    "scf.for %i = %c0 to %c_arr step %c1 {",
    "  %offset = arith.muli %i, %c8 : index",
    "  %row_addr = arith.addi %base_addr, %offset : index",
    "  memref.store %row_addr, %global_addrs[%i] : memref<16xindex>",
    "}"
  });

  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_simple_array = stack_memory_region_array->generate_in_other_f(
    array_region_canvas, "reallocated", 8, false
  );

  reused_region_canvas_simple_array->add_during_lifetime({
    "%precond_fail = arith.constant 43 : i32",
    "%test_success = arith.constant 42 : i32",
    "%c_arr = arith.constant 16 : index",
    "%c0 = arith.constant 0 : index",
    "%c1 = arith.constant 1 : index",
    "%realloc_addr = memref.extract_aligned_pointer_as_index %reallocated : memref<8xi8> -> index",
    "%global_addrs_other = memref.get_global @target_addresses : memref<16xindex>",
    "%false = arith.constant false",
    "%result:2 = scf.for %i = %c0 to %c_arr step %c1 iter_args(%found = %false, %idx = %c0) -> (i1, index) {",
    "  %target_addr = memref.load %global_addrs_other[%i] : memref<16xindex>",
    "  %eq = arith.cmpi eq, %realloc_addr, %target_addr : index",
    "  %next_found = arith.ori %found, %eq : i1",
    "  %next_idx = arith.select %eq, %i, %idx : index",
    "  scf.yield %next_found, %next_idx : i1, index",
    "}",
    "scf.if %result#0 {",
    "  %global_arr_access = memref.get_global @target_arr : memref<1xmemref<16x8xi8>>",
    "  %target_loaded = memref.load %global_arr_access[%c0] : memref<1xmemref<16x8xi8>>"
  });

  AccessLocation::SplitAccess access_type_code_array = access_location->generate_split_const_vars(
    access_action, "target_loaded", 8, 16, "result#1");

  reused_region_canvas_simple_array->add_during_lifetime(access_type_code_array.to_lines());
  reused_region_canvas_simple_array->add_during_lifetime({
    "  func.call @exit(%test_success) : (i32) -> ()",
    "  scf.yield",
    "} else {",
    "  func.call @exit(%precond_fail) : (i32) -> ()",
    "  scf.yield",
    "}"
  });
  reused_region_canvas_simple_array->add_variant_description_line("using an array of objects");
  variants.push_back( reused_region_canvas_simple_array );

  CodeCanvas code_array_repeated;
  code_array_repeated.add_global("memref.global @target_addresses : memref<16xindex>");
  code_array_repeated.add_global("memref.global @target_arr : memref<1xmemref<16x8xi8>>");
  code_array_repeated.add_global("func.func private @exit(%arg0: i32) -> ()");
  std::shared_ptr<StackRegion> stack_memory_region_array_repeated = std::make_shared<StackRegion>(*stack_memory_region);
  std::shared_ptr<RegionCodeCanvas> array_region_canvas_repeated = stack_memory_region_array_repeated->generate_array(std::make_shared<CodeCanvas>(code_array_repeated), "target", 8, 16, true);

  array_region_canvas_repeated->add_during_lifetime({
    "%global_arr = memref.get_global @target_arr : memref<1xmemref<16x8xi8>>",
    "memref.store %target, %global_arr[%c0] : memref<1xmemref<16x8xi8>>",
    "%base_addr = memref.extract_aligned_pointer_as_index %target : memref<16x8xi8> -> index",
    "%global_addrs = memref.get_global @target_addresses : memref<16xindex>",
    "scf.for %i = %c0 to %c_arr step %c1 {",
    "  %offset = arith.muli %i, %c8 : index",
    "  %row_addr = arith.addi %base_addr, %offset : index",
    "  memref.store %row_addr, %global_addrs[%i] : memref<16xindex>",
    "}"
  });

  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_array_repeated = stack_memory_region_array->generate_in_other_f(
    array_region_canvas_repeated, "reallocated", 8, false
  );
  reused_region_canvas_array_repeated->add_global("memref.global @last_address : memref<1xindex>");

  reused_region_canvas_array_repeated->add_during_lifetime({
    "%precond_fail = arith.constant 43 : i32",
    "%test_success = arith.constant 42 : i32",
    "%c_arr = arith.constant 16 : index",
    "%c0 = arith.constant 0 : index",
    "%c1 = arith.constant 1 : index",
    "%realloc_addr = memref.extract_aligned_pointer_as_index %reallocated : memref<8xi8> -> index",
    "%global_last = memref.get_global @last_address : memref<1xindex>",
    "%last_addr = memref.load %global_last[%c0] : memref<1xindex>",
    "%eq_last = arith.cmpi eq, %last_addr, %realloc_addr : index",
    "scf.if %eq_last {",
    "  func.call @exit(%precond_fail) : (i32) -> ()",
    "  scf.yield",
    "}",
    "memref.store %realloc_addr, %global_last[%c0] : memref<1xindex>",
    "%global_addrs_other = memref.get_global @target_addresses : memref<16xindex>",
    "%false = arith.constant false",
    "%result:2 = scf.for %i = %c0 to %c_arr step %c1 iter_args(%found = %false, %idx = %c0) -> (i1, index) {",
    "  %target_addr = memref.load %global_addrs_other[%i] : memref<16xindex>",
    "  %eq = arith.cmpi eq, %realloc_addr, %target_addr : index",
    "  %next_found = arith.ori %found, %eq : i1",
    "  %next_idx = arith.select %eq, %i, %idx : index",
    "  scf.yield %next_found, %next_idx : i1, index",
    "}",
    "scf.if %result#0 {",
    "  %global_arr_access = memref.get_global @target_arr : memref<1xmemref<16x8xi8>>",
    "  %target_loaded = memref.load %global_arr_access[%c0] : memref<1xmemref<16x8xi8>>"
  });

  AccessLocation::SplitAccess access_type_code_array_repeat = access_location->generate_split_const_vars(
    access_action, "target_loaded", 8, 16, "result#1");

  reused_region_canvas_array_repeated->add_during_lifetime(access_type_code_array_repeat.to_lines());
  reused_region_canvas_array_repeated->add_during_lifetime({
    "  func.call @exit(%test_success) : (i32) -> ()",
    "  scf.yield",
    "}"
  });

  reused_region_canvas_array_repeated->add_at(reused_region_canvas_array_repeated->get_other_f_call_pos(),
    std::vector<std::string>{
      "%c0_main = arith.constant 0 : index",
      "%c1_main = arith.constant 1 : index",
      "%cMAX_main = arith.constant " + max_reallocated_retries + " : index",
      "%results = scf.while (%counter = %c0_main) : (index) -> index {",
      "  %lt = arith.cmpi slt, %counter, %cMAX_main : index",
      "  scf.condition(%lt) %counter : index",
      "} do {",
      "^bb0(%counter_iter : index):"
    },
    "    "
  );
  reused_region_canvas_array_repeated->add_at(reused_region_canvas_array_repeated->get_other_f_call_pos() + 1,
    std::vector<std::string>{
      "  %next_counter = arith.addi %counter_iter, %c1_main : index",
      "  scf.yield %next_counter : index",
      "}",
      "%precond_fail = arith.constant 43 : i32",
      "func.call @exit(%precond_fail) : (i32) -> ()"
    },
    "    "
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
  code_simple.add_global("memref.global @target_address : memref<1xindex>");
  code_simple.add_global("memref.global @target_ptr : memref<1xmemref<8xi8>>");
  code_simple.add_global("func.func private @exit(%arg0: i32) -> ()");
  std::shared_ptr<StackRegion> stack_memory_region = std::dynamic_pointer_cast<StackRegion>(memory_region);
  std::shared_ptr<StackRegion> stack_memory_region_simple = std::make_shared<StackRegion>(*stack_memory_region);
  std::shared_ptr<RegionCodeCanvas> region_canvas = stack_memory_region_simple->generate(std::make_shared<CodeCanvas>(code_simple), "target", 8, true);

  region_canvas->add_to_f_body({
    "%target_addr = memref.extract_aligned_pointer_as_index %target : memref<8xi8> -> index",
    "%global_addr = memref.get_global @target_address : memref<1xindex>",
    "memref.store %target_addr, %global_addr[%c0] : memref<1xindex>",
    "%global_ptr = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "memref.store %target, %global_ptr[%c0] : memref<1xmemref<8xi8>>"
  });
  AccessLocation::SplitAccess access_type_code = access_location->generate_split_const_vars(
    access_action, "saved_ptr", 8);
  std::vector<std::string> lines = {
    "%global_ptr_main = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "%saved_ptr = memref.load %global_ptr_main[%c0] : memref<1xmemref<8xi8>>"
};
  region_canvas->add_to_f_body(lines);
  {
    std::vector<std::string> access_lines_with_aux = get_filtered_aux_lines(access_type_code);
    access_lines_with_aux.insert(access_lines_with_aux.end(), access_type_code.access_lines.begin(), access_type_code.access_lines.end());
    region_canvas->add_to_f_body(access_lines_with_aux);
  }
  region_canvas->add_to_f_body("func.call @exit(%test_success) : (i32) -> ()");

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
  code.add_global("memref.global @target_address : memref<1xindex>");
  code.add_global("memref.global @target_ptr : memref<1xmemref<8xi8>>");
  code.add_global("func.func private @exit(%arg0: i32) -> ()");
  std::shared_ptr<RegionCodeCanvas> region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(code), "target", 8, /*initialize=*/true);

  assert( is_a<HeapRegion>(memory_region) );

  // In validation version, access happens BEFORE deallocation (no actual free)
  auto index = region_canvas->add_to_f_body({
    "%target_addr = memref.extract_aligned_pointer_as_index %target : memref<8xi8> -> index",
    "%global_addr = memref.get_global @target_address : memref<1xindex>",
    "memref.store %target_addr, %global_addr[%c0] : memref<1xindex>",
    "%global_ptr = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "memref.store %target, %global_ptr[%c0] : memref<1xmemref<8xi8>>"
  });

  std::vector<std::string> access_type_code = access_location->generate(
    access_action, "saved_ptr", 8);
    std::vector<std::string> lines = {
    "%global_ptr_main = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "%saved_ptr = memref.load %global_ptr_main[%c0] : memref<1xmemref<8xi8>>"
};
  index = region_canvas->add_at(index,lines, "  ");
  index = region_canvas->add_at(index, access_type_code, "  ");
  region_canvas->add_at(index, "func.call @exit(%test_success) : (i32) -> ()", "  ");

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
  code.add_global("memref.global @target_address : memref<1xindex>");
  code.add_global("memref.global @target_ptr : memref<1xmemref<8xi8>>");
  code.add_global("func.func private @exit(%arg0: i32) -> ()");

  std::shared_ptr<RegionCodeCanvas> region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(code), "target", 8, true);

  assert ( is_a<HeapRegion>(memory_region) );

  region_canvas->add_during_lifetime({
    "%target_addr = memref.extract_aligned_pointer_as_index %target : memref<8xi8> -> index",
    "%global_addr = memref.get_global @target_address : memref<1xindex>",
    "memref.store %target_addr, %global_addr[%c0] : memref<1xindex>",
    "%global_ptr = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "memref.store %target, %global_ptr[%c0] : memref<1xmemref<8xi8>>"
  });

  std::vector<std::string> access_type_code = access_location->generate(
    access_action, "saved_ptr", 8);

  std::shared_ptr<HeapRegion> heap_memory_region = std::dynamic_pointer_cast<HeapRegion>(memory_region);

  std::shared_ptr<RegionCodeCanvas> reused_region_canvases = heap_memory_region->generate(
    region_canvas->get_lifetime_pos(), region_canvas, "reallocated_out", 8, false);
  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_simple = std::make_shared<RegionCodeCanvas>(*reused_region_canvases);
  std::vector<std::string> lines = {
    "%global_ptr_main = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "%saved_ptr = memref.load %global_ptr_main[%c0] : memref<1xmemref<8xi8>>"
};
  reused_region_canvas_simple->add_during_lifetime(lines);
  reused_region_canvas_simple->add_during_lifetime(access_type_code);
  reused_region_canvas_simple->add_during_lifetime("func.call @exit(%test_success) : (i32) -> ()");

  region_canvas->add_test_case_description_line("Memory region: heap");
  region_canvas->add_test_case_description_line("Bug type: use-after-*, reused memory");
  region_canvas->add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_repeat = std::make_shared<RegionCodeCanvas>(*reused_region_canvases);
  std::vector<std::string> allocation = heap_memory_region->generate_reallocation("reallocated", 8, true, "    ");
  std::vector<std::string> deallocation = heap_memory_region->generate_deallocation("reallocated", 8, "    ");

  reused_region_canvas_repeat->add_during_lifetime({
    "memref.dealloc %reallocated_out : memref<8xi8>",
    "%c0 = arith.constant 0 : index",
    "%c1 = arith.constant 1 : index",
    "%cMAX = arith.constant " + max_reallocated_retries_validation + " : index",
    "%false = arith.constant false",
    "%counter_final, %matched_final = scf.while (%counter = %c0, %matched = %false) : (index, i1) -> (index, i1) {",
    "  %lt = arith.cmpi slt, %counter, %cMAX : index",
    "  scf.condition(%lt) %counter, %matched : index, i1",
    "} do {",
    "  ^bb0(%counter_iter : index, %matched_iter : i1):"
  });
  reused_region_canvas_repeat->add_during_lifetime(allocation);
  reused_region_canvas_repeat->add_during_lifetime(deallocation);
  reused_region_canvas_repeat->add_during_lifetime({
    "    %next_counter = arith.addi %counter_iter, %c1 : index",
    "    scf.yield %next_counter, %matched_iter : index, i1",
    "}",
    "%global_ptr_access = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "%saved_ptr = memref.load %global_ptr_access[%c0] : memref<1xmemref<8xi8>>"
  });
  reused_region_canvas_repeat->add_during_lifetime(access_type_code);
  // reused_region_canvas_repeat->add_during_lifetime("func.return %test_success : i32");
  reused_region_canvas_repeat->add_during_lifetime("func.call @exit(%test_success) : (i32) -> ()");
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
  code_simple.add_global("memref.global @target_address : memref<1xindex>");
  code_simple.add_global("memref.global @target_ptr : memref<1xmemref<8xi8>>");
  code_simple.add_global("func.func private @exit(%arg0: i32) -> ()");
  code_simple.add_test_case_description_line("Memory region: stack");
  code_simple.add_test_case_description_line("Bug type: use-after-*, reused memory");
  code_simple.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  std::shared_ptr<StackRegion> stack_memory_region = std::dynamic_pointer_cast<StackRegion>(memory_region);
  std::shared_ptr<StackRegion> stack_memory_region_simple = std::make_shared<StackRegion>(*stack_memory_region);
  std::shared_ptr<RegionCodeCanvas> region_canvas = stack_memory_region_simple->generate(std::make_shared<CodeCanvas>(code_simple), "target", 8, true);

  region_canvas->add_to_f_body({
    "%target_addr = memref.extract_aligned_pointer_as_index %target : memref<8xi8> -> index",
    "%global_addr = memref.get_global @target_address : memref<1xindex>",
    "memref.store %target_addr, %global_addr[%c0] : memref<1xindex>",
    "%global_ptr = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "memref.store %target, %global_ptr[%c0] : memref<1xmemref<8xi8>>"
  });

  AccessLocation::SplitAccess access_type_code = access_location->generate_split_const_vars(
    access_action, "saved_ptr", 8);

  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_simple = stack_memory_region_simple->generate(
    region_canvas, "reallocated", 8, false
  );
  std::vector<std::string> lines = {
    "%global_ptr_main = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "%saved_ptr = memref.load %global_ptr_main[%c0] : memref<1xmemref<8xi8>>"
};
  reused_region_canvas_simple->add_to_f_body(lines);
  {
    std::vector<std::string> access_lines_with_aux = get_filtered_aux_lines(access_type_code);
    access_lines_with_aux.insert(access_lines_with_aux.end(), access_type_code.access_lines.begin(), access_type_code.access_lines.end());
    reused_region_canvas_simple->add_to_f_body(access_lines_with_aux);
  }
  reused_region_canvas_simple->add_to_f_body("func.call @exit(%test_success) : (i32) -> ()");
  variants.push_back( reused_region_canvas_simple );

  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_repeated = stack_memory_region_simple->generate(
    region_canvas, "reallocated", 8, false
    );
  reused_region_canvas_repeated->add_global("memref.global @last_address : memref<1xindex>");

  AccessLocation::SplitAccess access_type_code_repeat = access_location->generate_split_const_vars(
    access_action, "saved_ptr", 8);

  reused_region_canvas_repeated->add_to_f_body({
    "%realloc_addr = memref.extract_aligned_pointer_as_index %reallocated : memref<8xi8> -> index",
    "%global_last = memref.get_global @last_address : memref<1xindex>",
    "%last_addr = memref.load %global_last[%c0] : memref<1xindex>",
    "%eq_last = arith.cmpi eq, %last_addr, %realloc_addr : index",
    "scf.if %eq_last {",
    "  func.call @exit(%precond_fail) : (i32) -> ()",
    "  scf.yield",
    "}",
    "memref.store %realloc_addr, %global_last[%c0] : memref<1xindex>",
    "%global_ptr_access = memref.get_global @target_ptr : memref<1xmemref<8xi8>>",
    "%saved_ptr = memref.load %global_ptr_access[%c0] : memref<1xmemref<8xi8>>"
  });
  {
    std::vector<std::string> access_lines_with_aux = get_filtered_aux_lines(access_type_code_repeat);
    access_lines_with_aux.insert(access_lines_with_aux.end(), access_type_code_repeat.access_lines.begin(), access_type_code_repeat.access_lines.end());
    reused_region_canvas_repeated->add_to_f_body(access_lines_with_aux);
  }

  reused_region_canvas_repeated->add_at(reused_region_canvas_repeated->get_f_call_pos(),
    std::vector<std::string>{
      "%c0_main = arith.constant 0 : index",
      "%c1_main = arith.constant 1 : index",
      "%cMAX_main = arith.constant " + max_reallocated_retries_validation + " : index",
      "%results = scf.while (%counter = %c0_main) : (index) -> index {",
      "  %lt = arith.cmpi slt, %counter, %cMAX_main : index",
      "  scf.condition(%lt) %counter : index",
      "} do {",
      "^bb0(%counter_iter : index):"
    },
    "    "
  );
  reused_region_canvas_repeated->add_at(reused_region_canvas_repeated->get_f_call_pos() + 1,
    std::vector<std::string>{
      "  %next_counter = arith.addi %counter_iter, %c1_main : index",
      "  scf.yield %next_counter : index",
      "}",
      "%test_success = arith.constant 42 : i32",
      "func.call @exit(%test_success) : (i32) -> ()"
    },
    "    "
  );
  reused_region_canvas_repeated->add_variant_description_line("with repeated attempts");

  variants.push_back( reused_region_canvas_repeated );

  CodeCanvas code_array;
  code_array.add_global("memref.global @target_addresses : memref<16xindex>");
  code_array.add_global("memref.global @target_arr : memref<1xmemref<16x8xi8>>");
  code_array.add_global("func.func private @exit(%arg0: i32)  -> ()");
  std::shared_ptr<StackRegion> stack_memory_region_array = std::make_shared<StackRegion>(*stack_memory_region);
  std::shared_ptr<RegionCodeCanvas> array_region_canvas = stack_memory_region_array->generate_array(std::make_shared<CodeCanvas>(code_array), "target", 8, 16, true);

  array_region_canvas->add_to_f_body({
    "%global_arr = memref.get_global @target_arr : memref<1xmemref<16x8xi8>>",
    "memref.store %target, %global_arr[%c0] : memref<1xmemref<16x8xi8>>",
    "%base_addr = memref.extract_aligned_pointer_as_index %target : memref<16x8xi8> -> index",
    "%global_addrs = memref.get_global @target_addresses : memref<16xindex>",
    "scf.for %i = %c0 to %c_arr step %c1 {",
    "  %offset = arith.muli %i, %c8 : index",
    "  %row_addr = arith.addi %base_addr, %offset : index",
    "  memref.store %row_addr, %global_addrs[%i] : memref<16xindex>",
    "}"
  });

  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_simple_array = stack_memory_region_array->generate(
    array_region_canvas, "reallocated", 8, false
  );

  reused_region_canvas_simple_array->add_to_f_body({
    "%realloc_addr = memref.extract_aligned_pointer_as_index %reallocated : memref<8xi8> -> index",
    "%global_addrs_other = memref.get_global @target_addresses : memref<16xindex>",
    "%false = arith.constant false",
    "%result:2 = scf.for %i = %c0 to %c_arr step %c1 iter_args(%found = %false, %idx = %c0) -> (i1, index) {",
    "  %target_addr = memref.load %global_addrs_other[%i] : memref<16xindex>",
    "  %eq = arith.cmpi eq, %realloc_addr, %target_addr : index",
    "  %next_found = arith.ori %found, %eq : i1",
    "  %next_idx = arith.select %eq, %i, %idx : index",
    "  scf.yield %next_found, %next_idx : i1, index",
    "}",
    "%global_arr_access = memref.get_global @target_arr : memref<1xmemref<16x8xi8>>",
    "%target_loaded = memref.load %global_arr_access[%c0] : memref<1xmemref<16x8xi8>>"
  });

  AccessLocation::SplitAccess access_type_code_array = access_location->generate_split_const_vars(
    access_action, "target_loaded", 8, 16, "result#1");

  {
    std::vector<std::string> access_lines_with_aux = get_filtered_aux_lines(access_type_code_array);
    access_lines_with_aux.insert(access_lines_with_aux.end(), access_type_code_array.access_lines.begin(), access_type_code_array.access_lines.end());
    reused_region_canvas_simple_array->add_to_f_body(access_lines_with_aux);
  }
  reused_region_canvas_simple_array->add_to_f_body("func.call @exit(%test_success) : (i32) -> ()");
  reused_region_canvas_simple_array->add_variant_description_line("using an array of objects");

  variants.push_back( reused_region_canvas_simple_array );

  CodeCanvas code_array_repeated;
  code_array_repeated.add_global("memref.global @target_addresses : memref<16xindex>");
  code_array_repeated.add_global("memref.global @target_arr : memref<1xmemref<16x8xi8>>");
  code_array_repeated.add_global("func.func private @exit(%arg0: i32) -> ()");
  std::shared_ptr<StackRegion> stack_memory_region_array_repeated = std::make_shared<StackRegion>(*stack_memory_region);
  std::shared_ptr<RegionCodeCanvas> array_region_canvas_repeated = stack_memory_region_array_repeated->generate_array(std::make_shared<CodeCanvas>(code_array_repeated), "target", 8, 16, true);

  array_region_canvas_repeated->add_to_f_body({
    "%global_arr = memref.get_global @target_arr : memref<1xmemref<16x8xi8>>",
    "memref.store %target, %global_arr[%c0] : memref<1xmemref<16x8xi8>>",
    "%base_addr = memref.extract_aligned_pointer_as_index %target : memref<16x8xi8> -> index",
    "%global_addrs = memref.get_global @target_addresses : memref<16xindex>",
    "scf.for %i = %c0 to %c_arr step %c1 {",
    "  %offset = arith.muli %i, %c8 : index",
    "  %row_addr = arith.addi %base_addr, %offset : index",
    "  memref.store %row_addr, %global_addrs[%i] : memref<16xindex>",
    "}"
  });

  std::shared_ptr<RegionCodeCanvas> reused_region_canvas_array_repeated = stack_memory_region_array_repeated->generate(
    array_region_canvas_repeated, "reallocated", 8, false
  );
  reused_region_canvas_array_repeated->add_global("memref.global @last_address : memref<1xindex>");

  reused_region_canvas_array_repeated->add_to_f_body({
    "%realloc_addr = memref.extract_aligned_pointer_as_index %reallocated : memref<8xi8> -> index",
    "%global_last = memref.get_global @last_address : memref<1xindex>",
    "%last_addr = memref.load %global_last[%c0] : memref<1xindex>",
    "%eq_last = arith.cmpi eq, %last_addr, %realloc_addr : index",
    "scf.if %eq_last {",
    "  func.call @exit(%precond_fail) : (i32) -> ()",
    "  scf.yield",
    "}",
    "memref.store %realloc_addr, %global_last[%c0] : memref<1xindex>",
    "%global_addrs_other = memref.get_global @target_addresses : memref<16xindex>",
    "%false = arith.constant false",
    "%result:2 = scf.for %i = %c0 to %c_arr step %c1 iter_args(%found = %false, %idx = %c0) -> (i1, index) {",
    "  %target_addr = memref.load %global_addrs_other[%i] : memref<16xindex>",
    "  %eq = arith.cmpi eq, %realloc_addr, %target_addr : index",
    "  %next_found = arith.ori %found, %eq : i1",
    "  %next_idx = arith.select %eq, %i, %idx : index",
    "  scf.yield %next_found, %next_idx : i1, index",
    "}",
    "%global_arr_access = memref.get_global @target_arr : memref<1xmemref<16x8xi8>>",
    "%target_loaded = memref.load %global_arr_access[%c0] : memref<1xmemref<16x8xi8>>"
  });

  AccessLocation::SplitAccess access_type_code_array_repeat = access_location->generate_split_const_vars(
    access_action, "target_loaded", 8, 16, "result#1");

  {
    std::vector<std::string> access_lines_with_aux = get_filtered_aux_lines(access_type_code_array_repeat);
    access_lines_with_aux.insert(access_lines_with_aux.end(), access_type_code_array_repeat.access_lines.begin(), access_type_code_array_repeat.access_lines.end());
    reused_region_canvas_array_repeated->add_to_f_body(access_lines_with_aux);
  }

  reused_region_canvas_array_repeated->add_at(reused_region_canvas_array_repeated->get_f_call_pos(),
    std::vector<std::string>{
      "%c0_main = arith.constant 0 : index",
      "%c1_main = arith.constant 1 : index",
      "%cMAX_main = arith.constant " + max_reallocated_retries_validation + " : index",
      "%results = scf.while (%counter = %c0_main) : (index) -> index {",
      "  %lt = arith.cmpi slt, %counter, %cMAX_main : index",
      "  scf.condition(%lt) %counter : index",
      "} do {",
      "^bb0(%counter_iter : index):"
    },
    "    "
  );
  reused_region_canvas_array_repeated->add_at(reused_region_canvas_array_repeated->get_f_call_pos() + 1,
    std::vector<std::string>{
      "  %next_counter = arith.addi %counter_iter, %c1_main : index",
      "  scf.yield %next_counter : index",
      "}",
      "%test_success = arith.constant 42 : i32",
      "func.call @exit(%test_success) : (i32) -> ()"
    },
    "    "
  );
  reused_region_canvas_array_repeated->add_variant_description_line("with repeated attempts");
  reused_region_canvas_array_repeated->add_variant_description_line("using an array of objects");

  variants.push_back( reused_region_canvas_array_repeated );

  return variants;
}
