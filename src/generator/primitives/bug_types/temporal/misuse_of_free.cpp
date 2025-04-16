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
    <target_allocation> <- size 160
    <target_name>[8] = 0x40; <- 0x20 for unused
    <target_name>[13*8] = 0x40;
    #ifndef __GLIBC__
    exit(PRECONDITIONS_FAILED_VALUE); // not using glibc
    #endif
    unsigned long *crafted_ptr;
    crafted_ptr = &(<target_name>[2*8]); // pointing at byte 0x10, content of chunk 0
    free(crafted_ptr);

    char* heap_obj;
    heap_obj = (char *)malloc(8);

    if ( &<target_name>[2*8] != heap_obj ) _exit(PRECONDITIONS_FAILED_VALUE); <- only for used
    <action>

    _exit(42);
    <target_deallocation>
  */

  CodeCanvas code;

  std::vector< std::string > magic_values = {"0x20", "0x40", "0x60"};
  std::shared_ptr<RegionCodeCanvas> region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(code), "target", 160, false);

  for ( const std::string &magic_value: magic_values)
  {
    std::shared_ptr<RegionCodeCanvas> region_canvas_with_magic_value = std::make_shared<RegionCodeCanvas>(*region_canvas);
    region_canvas_with_magic_value->add_global("char* heap_obj;");
    region_canvas_with_magic_value->add_during_lifetime({
      "#ifndef __GLIBC__",
      "exit(PRECONDITIONS_FAILED_VALUE); // not using glibc",
      "#endif",
      "for (size_t i = 0; i < 16; i++) target[i] = 0;",
      "target[8] = " + magic_value + "; // magic value",
      "target[13*8] = 0x40;",
      "unsigned long *crafted_ptr;",
      "crafted_ptr = (unsigned long *)&(target[2*8]); // pointing at byte 0x10, content of chunk 0",
      "(void)malloc(8);",
      "free(crafted_ptr);",
        "",
        "heap_obj = (char *)malloc(8);"
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
        // unused heap memory
        region_canvas_with_magic_value->add_at(
          region_canvas_with_magic_value->get_deallocation_pos() - 1,
          std::vector<std::string>{
            "if (GET_ADDR_BITS(target) == GET_ADDR_BITS(crafted_ptr))",
            "{"
          },
          "  "
          );
        index = region_canvas_with_magic_value->add_at(
          region_canvas_with_magic_value->get_deallocation_pos(),
          std::vector<std::string>{
            "}"
          },
          "  "
        );
        index = region_canvas_with_magic_value->add_at(index, access_type_code, "  ");
      }
      else
      {
        // unused memory, but not on heap
        index = region_canvas_with_magic_value->add_at(region_canvas_with_magic_value->get_deallocation_pos(), access_type_code, "  ");
      }
    }
    region_canvas_with_magic_value->add_at(index, "_exit(TEST_CASE_SUCCESSFUL_VALUE);", "  ");
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
    <target_allocation> <- size 160
    <target_name>[8] = 0x40; <- 0x20 for unused
    <target_name>[13*8] = 0x40;
    #ifndef __GLIBC__
    exit(PRECONDITIONS_FAILED_VALUE); // not using glibc
    #endif
    unsigned long *crafted_ptr;
    crafted_ptr = &(<target_name>[2*8]); // pointing at byte 0x10, content of chunk 0

    char* heap_obj;
    heap_obj = (char *)malloc(8);

    <action>

    _exit(42);
    <target_deallocation>
  */

  CodeCanvas code;

  std::vector< std::string > magic_values = {"0x20", "0x40", "0x60"};
  std::shared_ptr<RegionCodeCanvas> region_canvas = memory_region->generate(std::make_shared<CodeCanvas>(code), "target", 160, false);

  for ( const std::string &magic_value: magic_values)
  {
    std::shared_ptr<RegionCodeCanvas> region_canvas_with_magic_value = std::make_shared<RegionCodeCanvas>(*region_canvas);
    region_canvas_with_magic_value->add_global("char* heap_obj;");
    region_canvas_with_magic_value->add_during_lifetime({
      "#ifndef __GLIBC__",
      "exit(PRECONDITIONS_FAILED_VALUE); // not using glibc",
      "#endif",
      "for (size_t i = 0; i < 16; i++) target[i] = 0;",
      "target[8] = " + magic_value + "; // magic value",
      "target[13*8] = 0x40;",
      "unsigned long *crafted_ptr;",
      "crafted_ptr = (unsigned long *)&(target[2*8]); // pointing at byte 0x10, content of chunk 0",
      "(void)malloc(8);",
      "(void)crafted_ptr;",
      "",
      "heap_obj = (char *)malloc(8);"
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
      index = region_canvas_with_magic_value->add_at(region_canvas_with_magic_value->get_deallocation_pos(), access_type_code, "  ");
    }
    region_canvas_with_magic_value->add_at(index, "_exit(TEST_CASE_SUCCESSFUL_VALUE);", "  ");
    full_variants.push_back( region_canvas_with_magic_value );
  }

  return full_variants;
}
