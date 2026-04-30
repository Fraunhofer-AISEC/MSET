/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */


#include "stack_region.h"

#include <cassert>

StackRegion::StackRegion():
  Region("stack")
{
}

std::shared_ptr<RegionCodeCanvas> StackRegion::generate(std::shared_ptr<CodeCanvas> code_canvas, std::string name, size_t size, bool initialize) const
{
  std::shared_ptr<RegionCodeCanvas> populated_code_canvas = std::make_shared<RegionCodeCanvas>(*code_canvas, std::to_string(size) );
  CodeCanvas::code_pos_t allocation_pos = populated_code_canvas->add_local(
    "%" + name + " = memref.alloca() : memref<" + std::to_string(size) + "xi8>"
  );
  CodeCanvas::code_pos_t current = allocation_pos;
  if (initialize)
  {
    current = _generate_init_loop(populated_code_canvas, current, name, size, "0xAA", "    ");
  }
  populated_code_canvas->set_allocation_pos(allocation_pos - 1);
  populated_code_canvas->set_deallocation_pos(CodeCanvas::INVALID_CODE_POS);
  populated_code_canvas->set_lifetime_pos(current);

  return populated_code_canvas;
}

std::shared_ptr<RegionCodeCanvas> StackRegion::generate(CodeCanvas::code_pos_t pos, std::shared_ptr<CodeCanvas> code_canvas, std::string name, size_t size, bool initialize) const
{
  return generate(code_canvas, name, size, initialize);
}

std::shared_ptr<RegionCodeCanvas> StackRegion::generate_in_other_f(std::shared_ptr<CodeCanvas> canvas, std::string name, size_t size,
  bool initialize) const
{
  std::shared_ptr<RegionCodeCanvas> populated_code_canvas = std::make_shared<RegionCodeCanvas>(*canvas, size);
  CodeCanvas::code_pos_t allocation_pos = populated_code_canvas->add_to_other_f_body(
    "%" + name + " = memref.alloca() : memref<" + std::to_string(size) + "xi8>"
  );
  CodeCanvas::code_pos_t current = allocation_pos;
  if (initialize)
  {
    current = _generate_init_loop(populated_code_canvas, current, name, size, "0xAA", "    ");
  }
  populated_code_canvas->set_allocation_pos(allocation_pos - 1);
  populated_code_canvas->set_deallocation_pos(CodeCanvas::INVALID_CODE_POS);
  populated_code_canvas->set_lifetime_pos(current);

  return populated_code_canvas;

}

std::shared_ptr<RegionCodeCanvas> StackRegion::generate_array(std::shared_ptr<CodeCanvas> canvas, std::string name, size_t size, size_t array_size,
  bool initialize) const
{
  std::shared_ptr<RegionCodeCanvas> populated_code_canvas = std::make_shared<RegionCodeCanvas>(*canvas, size);
  CodeCanvas::code_pos_t allocation_pos = populated_code_canvas->add_to_f_body(
    "%" + name + " = memref.alloca() : memref<" + std::to_string(array_size) + "x" + std::to_string(size) + "xi8>"
  );
  CodeCanvas::code_pos_t current = allocation_pos;
  if (initialize)
  {
    current = _generate_2d_init_loop(populated_code_canvas, current, name, size, array_size, "0xAA", "    ");
  }
  populated_code_canvas->set_allocation_pos(allocation_pos - 1);
  populated_code_canvas->set_deallocation_pos(CodeCanvas::INVALID_CODE_POS);
  populated_code_canvas->set_lifetime_pos(current);

  return populated_code_canvas;

}

std::shared_ptr<RegionCodeCanvas> StackRegion::generate(
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
  std::shared_ptr<RegionCodeCanvas> populated_code_canvas = std::make_shared<RegionCodeCanvas>(*canvas, "sizeof(struct T)");

  // Allocate parent memref
  CodeCanvas::code_pos_t allocation_pos = populated_code_canvas->add_local(
    "%" + name + " = memref.alloca() : memref<" + std::to_string(total_size) + "xi8>"
  );

  // Create subviews for fields
  CodeCanvas::code_pos_t current = populated_code_canvas->add_to_f_body(
    "%" + name + "_" + name_field_1 + " = memref.subview %" + name + "[0][" + std::to_string(size_field_1) + "][1] : memref<" + std::to_string(total_size) + "xi8> to memref<" + std::to_string(size_field_1) + "xi8>"
  );
  current = populated_code_canvas->add_to_f_body(
    "%" + name + "_" + name_field_2 + " = memref.subview %" + name + "[" + std::to_string(size_field_1 + gap) + "][" + std::to_string(size_field_2) + "][1] : memref<" + std::to_string(total_size) + "xi8> to memref<" + std::to_string(size_field_2) + "xi8>"
  );

  if (initialize)
  {
    current = _generate_init_loop(populated_code_canvas, current, name + "_" + name_field_1, size_field_1, "0xAA", "    ");
    current = _generate_init_loop(populated_code_canvas, current, name + "_" + name_field_2, size_field_2, "0xBB", "    ");
  }
  populated_code_canvas->set_allocation_pos(allocation_pos - 1);
  populated_code_canvas->set_deallocation_pos(CodeCanvas::INVALID_CODE_POS);
  populated_code_canvas->set_lifetime_pos(current);

  return populated_code_canvas;
}

CodeCanvas::code_pos_t StackRegion::_generate_init_loop(
  std::shared_ptr<RegionCodeCanvas> canvas,
  CodeCanvas::code_pos_t where,
  const std::string &name,
  size_t size,
  const std::string &value,
  const std::string &indent
) const
{
  std::vector<std::string> loop = {
    "%c0 = arith.constant 0 : index",
    "%c1 = arith.constant 1 : index",
    "%c" + std::to_string(size) + " = arith.constant " + std::to_string(size) + " : index",
    "%c" + value + " = arith.constant " + std::to_string(std::stoi(value, nullptr, 16)) + " : i8",
    "scf.for %i = %c0 to %c" + std::to_string(size) + " step %c1 {",
    "  memref.store %c" + value + ", %" + name + "[%i] : memref<" + std::to_string(size) + "xi8>",
    "}"
  };
  return canvas->add_at(where, loop, indent);
}

CodeCanvas::code_pos_t StackRegion::_generate_2d_init_loop(
  std::shared_ptr<RegionCodeCanvas> canvas,
  CodeCanvas::code_pos_t where,
  const std::string &name,
  size_t size,
  size_t array_size,
  const std::string &value,
  const std::string &indent
) const
{
  std::vector<std::string> loop = {
    "%c0 = arith.constant 0 : index",
    "%c1 = arith.constant 1 : index",
    "%c_arr = arith.constant " + std::to_string(array_size) + " : index",
    "%c_sz = arith.constant " + std::to_string(size) + " : index",
    "%c" + value + " = arith.constant " + std::to_string(std::stoi(value, nullptr, 16)) + " : i8",
    "scf.for %i = %c0 to %c_arr step %c1 {",
    "  scf.for %j = %c0 to %c_sz step %c1 {",
    "    memref.store %c" + value + ", %" + name + "[%i, %j] : memref<" + std::to_string(array_size) + "x" + std::to_string(size) + "xi8>",
    "  }",
    "}"
  };
  return canvas->add_at(where, loop, indent);
}
