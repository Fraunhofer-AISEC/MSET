/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "misuse_of_free.h"

#include "misc.h"
#include "generator/primitives/bug_types/temporal/memory_state/used.h"
#include "generator/primitives/regions/heap_region.h"

MisuseOfFree::MisuseOfFree():
  TemporalBugType("misuse_of_free")
{
}

bool MisuseOfFree::accepts(std::shared_ptr<MemoryState> memory_state)
{
  return true;
}

bool MisuseOfFree::accepts(std::shared_ptr<Region> region)
{
  return true;
}

bool MisuseOfFree::accepts(std::shared_ptr<AccessLocation> access_location)
{
  return true;
}

std::vector< std::shared_ptr<RegionCodeCanvas> >MisuseOfFree::generate(
  std::shared_ptr<MemoryState> memory_state,
  std::shared_ptr<Region> memory_region,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location
  ) const
{
  std::vector< std::shared_ptr<RegionCodeCanvas> > full_variants;
  /*
    %target = memref.alloc() : memref<160xi8>
    memref.store %c_magic, %target[%c8] : memref<160xi8>
    memref.store %c0x40, %target[%c104] : memref<160xi8>
    %crafted = memref.subview %target[16][8][1] : memref<160xi8> to memref<8xi8, strided<[1], offset: 16>>
    %_ = memref.alloc() : memref<8xi8>
    memref.dealloc %crafted : memref<8xi8, strided<[1], offset: 16>>

    %heap_obj = memref.alloc() : memref<8xi8>

    scf.if %eq_cmp { // only for unused heap
      <action>
    }

    func.return %test_success : i32
    memref.dealloc %target : memref<160xi8>
  */

  CodeCanvas code;
  code.add_global("func.func private @exit(%arg0: i32) -> ()");

  code.add_test_case_description_line("Memory region: " + memory_region->get_name());
  code.add_test_case_description_line("Bug type: misuse-of-free, " + memory_state->get_printable_name());
  code.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());

  std::vector< std::string > magic_values = {"0x20", "0x40", "0x60"};
  std::shared_ptr<RegionCodeCanvas> region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(code), "target", 160, false);

  for ( const std::string &magic_value: magic_values)
  {
    std::shared_ptr<RegionCodeCanvas> region_canvas_with_magic_value = std::make_shared<RegionCodeCanvas>(*region_canvas);

    region_canvas_with_magic_value->add_variant_description_line("magic value " + magic_value);

    region_canvas_with_magic_value->add_during_lifetime({
      "%c8_mof = arith.constant 8 : index",
      "%c104_mof = arith.constant 104 : index",
      "%c_magic = arith.constant " + std::to_string(std::stoi(magic_value, nullptr, 16)) + " : i8",
      "%c0x40 = arith.constant 64 : i8",
      "memref.store %c_magic, %target[%c8_mof] : memref<160xi8> // magic value",
      "memref.store %c0x40, %target[%c104_mof] : memref<160xi8>",
      "%crafted = memref.subview %target[16][8][1] : memref<160xi8> to memref<8xi8, strided<[1], offset: 16>>",
      "%_ = memref.alloc() : memref<8xi8>",
      "memref.dealloc %crafted : memref<8xi8, strided<[1], offset: 16>>",
      "",
      "%heap_obj = memref.alloc() : memref<8xi8>"
    });
    std::vector<std::string> access_type_code = access_location->generate(
      access_action, "heap_obj", 8);
    CodeCanvas::code_pos_t index;
    if (is_a<UsedMemory>(memory_state))
    {
      index = region_canvas_with_magic_value->add_at(region_canvas_with_magic_value->get_lifetime_pos(), access_type_code, "  ");
    }
    else
    {
      if ( std::dynamic_pointer_cast<HeapRegion>(memory_region) )
      {
        // unused heap memory: conditionally dealloc target, access is outside if
        std::vector<std::string> cmp_and_if = {
          "%target_addr_cmp = memref.extract_aligned_pointer_as_index %target : memref<160xi8> -> index",
          "%crafted_addr_cmp = memref.extract_aligned_pointer_as_index %crafted : memref<8xi8, strided<[1], offset: 16>> -> index",
          "%eq_cmp = arith.cmpi eq, %target_addr_cmp, %crafted_addr_cmp : index",
          "scf.if %eq_cmp {"
        };
        // Insert comparison and if-start before deallocation
        region_canvas_with_magic_value->add_at(
          region_canvas_with_magic_value->get_deallocation_pos() - 1,
          cmp_and_if,
          "  "
        );
        // Insert closing brace after deallocation
        auto after_brace = region_canvas_with_magic_value->add_at(
          region_canvas_with_magic_value->get_deallocation_pos(),
          "}",
          "  "
        );
        // Insert access code after the if block
        index = region_canvas_with_magic_value->add_at(after_brace, access_type_code, "  ");
      }
      else
      {
        // unused memory, but not on heap (stack has no deallocation_pos, use lifetime_pos instead)
        index = region_canvas_with_magic_value->add_at(region_canvas_with_magic_value->get_lifetime_pos(), access_type_code, "  ");
      }
    }
    region_canvas_with_magic_value->add_at(index, "func.call @exit(%test_success) : (i32) -> ()", "  ");
    full_variants.push_back( region_canvas_with_magic_value );
  }

  return full_variants;
}


std::vector< std::shared_ptr<RegionCodeCanvas> >MisuseOfFree::generate_validation(
  std::shared_ptr<MemoryState> memory_state,
  std::shared_ptr<Region> memory_region,
  std::shared_ptr<AccessAction> access_action,
  std::shared_ptr<AccessLocation> access_location
  ) const
{
  std::vector< std::shared_ptr<RegionCodeCanvas> > full_variants;
  /*
    %target = memref.alloc() : memref<160xi8>
    memref.store %c_magic, %target[%c8] : memref<160xi8>
    memref.store %c0x40, %target[%c104] : memref<160xi8>
    %crafted = memref.subview %target[16][8][1] : memref<160xi8> to memref<8xi8, strided<[1], offset: 16>>
    %_ = memref.alloc() : memref<8xi8>

    %heap_obj = memref.alloc() : memref<8xi8>

    <action>

    func.return %test_success : i32
    memref.dealloc %target : memref<160xi8>
  */

  CodeCanvas code;
  code.add_test_case_description_line("Memory region: " + memory_region->get_name());
  code.add_test_case_description_line("Bug type: misuse-of-free, " + memory_state->get_printable_name());
  code.add_test_case_description_line("Access type: " + access_location->get_name() + ", " + access_action->get_name());


  std::vector< std::string > magic_values = {"0x20", "0x40", "0x60"};
  std::shared_ptr<RegionCodeCanvas> region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(code), "target", 160, false);

  for ( const std::string &magic_value: magic_values)
  {
    std::shared_ptr<RegionCodeCanvas> region_canvas_with_magic_value = std::make_shared<RegionCodeCanvas>(*region_canvas);

    region_canvas_with_magic_value->add_variant_description_line("magic value " + magic_value);

    region_canvas_with_magic_value->add_during_lifetime({
      "%c8_mof = arith.constant 8 : index",
      "%c104_mof = arith.constant 104 : index",
      "%c_magic = arith.constant " + std::to_string(std::stoi(magic_value, nullptr, 16)) + " : i8",
      "%c0x40 = arith.constant 64 : i8",
      "memref.store %c_magic, %target[%c8_mof] : memref<160xi8> // magic value",
      "memref.store %c0x40, %target[%c104_mof] : memref<160xi8>",
      "%crafted = memref.subview %target[16][8][1] : memref<160xi8> to memref<8xi8, strided<[1], offset: 16>>",
      "%_ = memref.alloc() : memref<8xi8>",
      "",
      "%heap_obj = memref.alloc() : memref<8xi8>"
    });
    std::vector<std::string> access_type_code = access_location->generate(
      access_action, "heap_obj", 8);
    CodeCanvas::code_pos_t index;
    if (is_a<UsedMemory>(memory_state))
    {
      index = region_canvas_with_magic_value->add_at(region_canvas_with_magic_value->get_lifetime_pos(), access_type_code, "  ");
    }
    else
    {
      if ( std::dynamic_pointer_cast<HeapRegion>(memory_region) )
      {
        index = region_canvas_with_magic_value->add_at(region_canvas_with_magic_value->get_deallocation_pos(), access_type_code, "  ");
      }
      else
      {
        index = region_canvas_with_magic_value->add_at(region_canvas_with_magic_value->get_lifetime_pos(), access_type_code, "  ");
      }
    }
    region_canvas_with_magic_value->add_at(index, "func.call @exit(%test_success) : (i32) -> ()", "  ");
    full_variants.push_back( region_canvas_with_magic_value );
  }

  return full_variants;
}
