/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "heap_region.h"

#include <cassert>
#include <iostream>
#include <ostream>

HeapRegion::HeapRegion():
  Region("heap")
{
}

std::shared_ptr<RegionCodeCanvas> HeapRegion::generate(CodeCanvas::code_pos_t where, std::shared_ptr<CodeCanvas> canvas, std::string name, size_t size, bool initialize) const
{
  std::shared_ptr<RegionCodeCanvas> populated_code_canvas = std::make_shared<RegionCodeCanvas>(*canvas, size);

  where = populated_code_canvas->add_at(
    where,
    "%" + name + " = memref.alloc() : memref<" + std::to_string(size) + "xi8>",
    "    "
  );
  CodeCanvas::code_pos_t allocation_pos = where;

  CodeCanvas::code_pos_t lifetime_pos;
  if (initialize)
  {
    lifetime_pos = _generate_init_loop(populated_code_canvas, where, name, size, "0xAA", "    ");
  }
  else
  {
    lifetime_pos = populated_code_canvas->add_at(where, "");
  }
  CodeCanvas::code_pos_t deallocation_pos = populated_code_canvas->add_to_f_body_end(
    "memref.dealloc %" + name + " : memref<" + std::to_string(size) + "xi8>"
  );
  populated_code_canvas->set_allocation_pos(allocation_pos);
  populated_code_canvas->set_deallocation_pos(deallocation_pos);
  populated_code_canvas->set_lifetime_pos(lifetime_pos);
  return populated_code_canvas;
}

std::shared_ptr<RegionCodeCanvas> HeapRegion::generate(std::shared_ptr<CodeCanvas> canvas, std::string name, size_t size, bool initialize) const
{
  std::shared_ptr<RegionCodeCanvas> populated_code_canvas = std::make_shared<RegionCodeCanvas>(*canvas, size);

  CodeCanvas::code_pos_t allocation_pos = populated_code_canvas->add_to_f_body(
    "%" + name + " = memref.alloc() : memref<" + std::to_string(size) + "xi8>"
  );

  CodeCanvas::code_pos_t lifetime_pos = allocation_pos;
  if (initialize)
  {
    lifetime_pos = _generate_init_loop(populated_code_canvas, lifetime_pos, name, size, "0xAA", "    ");
  }
  else
  {
    lifetime_pos = populated_code_canvas->add_to_f_body("");
  }
  CodeCanvas::code_pos_t deallocation_pos = populated_code_canvas->add_to_f_body_end(
    "memref.dealloc %" + name + " : memref<" + std::to_string(size) + "xi8>"
  );
  populated_code_canvas->set_allocation_pos(allocation_pos);
  populated_code_canvas->set_deallocation_pos(deallocation_pos);
  populated_code_canvas->set_lifetime_pos(lifetime_pos);
  return populated_code_canvas;
}


std::shared_ptr<RegionCodeCanvas> HeapRegion::generate(
  std::shared_ptr<CodeCanvas> canvas,
  std::string name,
  std::string name_field_1, size_t size_field_1,
  std::string name_field_2, size_t size_field_2,
  bool initialize,
  size_t gap
) const
{
  assert(size_field_1); assert(size_field_2);
  size_t total_size = size_field_1 + gap + size_field_2;
  std::shared_ptr<RegionCodeCanvas> populated_code_canvas = std::make_shared<RegionCodeCanvas>(*canvas, total_size);

  // Allocate parent memref
  CodeCanvas::code_pos_t allocation_pos = populated_code_canvas->add_to_f_body(
    "%" + name + " = memref.alloc() : memref<" + std::to_string(total_size) + "xi8>"
  );

  // Create subviews for fields
  CodeCanvas::code_pos_t current = populated_code_canvas->add_to_f_body(
    "%" + name + "_" + name_field_1 + " = memref.subview %" + name + "[0][" + std::to_string(size_field_1) + "][1] : memref<" + std::to_string(total_size) + "xi8> to memref<" + std::to_string(size_field_1) + "xi8, strided<[1], offset: 0>>"
  );
  current = populated_code_canvas->add_to_f_body(
    "%" + name + "_" + name_field_2 + " = memref.subview %" + name + "[" + std::to_string(size_field_1 + gap) + "][" + std::to_string(size_field_2) + "][1] : memref<" + std::to_string(total_size) + "xi8> to memref<" + std::to_string(size_field_2) + "xi8, strided<[1], offset: " + std::to_string(size_field_1 + gap) + ">>"
  );

  CodeCanvas::code_pos_t lifetime_pos = current;
  if (initialize)
  {
    lifetime_pos = _generate_init_loop(populated_code_canvas, current, name + "_" + name_field_1, size_field_1, "0xAA", "    ", true, "0");
    lifetime_pos = _generate_init_loop(populated_code_canvas, lifetime_pos, name + "_" + name_field_2, size_field_2, "0xBB", "    ", true, std::to_string(size_field_1 + gap));
  }
  else
  {
    lifetime_pos = populated_code_canvas->add_to_f_body("");
  }
  CodeCanvas::code_pos_t deallocation_pos = populated_code_canvas->add_to_f_body_end(
    "memref.dealloc %" + name + " : memref<" + std::to_string(total_size) + "xi8>"
  );
  populated_code_canvas->set_allocation_pos(allocation_pos);
  populated_code_canvas->set_deallocation_pos(deallocation_pos);
  populated_code_canvas->set_lifetime_pos(lifetime_pos);
  return populated_code_canvas;
}


CodeCanvas::code_pos_t HeapRegion::_generate_init_loop(
  std::shared_ptr<RegionCodeCanvas> canvas,
  CodeCanvas::code_pos_t where,
  const std::string &name,
  size_t size,
  const std::string &value,
  const std::string &indent,
  bool needs_strided,
  const std::string &offset
) const
{
  std::string type_suffix = needs_strided
    ? ", strided<[1], offset: " + offset + ">>"
    : ">";
  // Generate scf.for loop for initialization
  std::vector<std::string> loop = {
    "%c0 = arith.constant 0 : index",
    "%c1 = arith.constant 1 : index",
    "%c" + std::to_string(size) + " = arith.constant " + std::to_string(size) + " : index",
    "%c" + value + " = arith.constant " + std::to_string(std::stoi(value, nullptr, 16)) + " : i8",
    "scf.for %i = %c0 to %c" + std::to_string(size) + " step %c1 {",
    "  memref.store %c" + value + ", %" + name + "[%i] : memref<" + std::to_string(size) + "xi8" + type_suffix,
    "}"
  };
  return canvas->add_at(where, loop, indent);
}

std::vector<std::string> HeapRegion::generate_reallocation(std::string name, size_t size, bool initialize, std::string indent) const
{
  std::vector<std::string> reallocation = {indent + "%" + name + " = memref.alloc() : memref<" + std::to_string(size) + "xi8>"};

  if (initialize)
  {
    reallocation.push_back(indent + "%c0 = arith.constant 0 : index");
    reallocation.push_back(indent + "%c1 = arith.constant 1 : index");
    reallocation.push_back(indent + "%c" + std::to_string(size) + " = arith.constant " + std::to_string(size) + " : index");
    reallocation.push_back(indent + "%c0xAA = arith.constant 170 : i8");
    reallocation.push_back(indent + "scf.for %i = %c0 to %c" + std::to_string(size) + " step %c1 {");
    reallocation.push_back(indent + "  memref.store %c0xAA, %" + name + "[%i] : memref<" + std::to_string(size) + "xi8>");
    reallocation.push_back(indent + "}");
  }
  return reallocation;
}

std::vector<std::string> HeapRegion::generate_deallocation(std::string name, size_t size, std::string indent) const
{
  return {indent + "memref.dealloc %" + name + " : memref<" + std::to_string(size) + "xi8>"};
}
