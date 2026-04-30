/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "global_region.h"

#include <cassert>

GlobalRegion::GlobalRegion():
  Region("global")
{
}

std::shared_ptr<RegionCodeCanvas> GlobalRegion::generate(std::shared_ptr<CodeCanvas> canvas, std::string name, size_t size, bool initialize) const
{
  assert(size);
  std::shared_ptr<RegionCodeCanvas> populated_code_canvas = std::make_shared<RegionCodeCanvas>(*canvas, size);
  std::string definition = "memref.global @" + name + " : memref<" + std::to_string(size) + "xi8> = dense<";
  if (initialize)
  {
    definition += "170>";
  }
  else
  {
    definition += "0>";
  }
  auto it = populated_code_canvas->add_global(definition);
  
  // In function body, get_global to use it
  populated_code_canvas->add_to_f_body(
    "%" + name + " = memref.get_global @" + name + " : memref<" + std::to_string(size) + "xi8>"
  );
  
  populated_code_canvas->set_allocation_pos(it);
  populated_code_canvas->set_deallocation_pos(CodeCanvas::INVALID_CODE_POS);
  populated_code_canvas->set_lifetime_pos(populated_code_canvas->get_current_pos_in_f());
  return populated_code_canvas;
}

std::shared_ptr<RegionCodeCanvas> GlobalRegion::generate(CodeCanvas::code_pos_t pos, std::shared_ptr<CodeCanvas> canvas, std::string name, size_t size, bool initialize) const
{
  return generate(canvas, name, size, initialize);
}

std::shared_ptr<RegionCodeCanvas> GlobalRegion::generate(
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
  
  std::string definition = "memref.global @" + name + " : memref<" + std::to_string(total_size) + "xi8> = dense<";
  if (initialize)
  {
    definition += "170>";
  }
  else
  {
    definition += "0>";
  }
  auto it = populated_code_canvas->add_global(definition);
  
  // In function body, get_global and create subviews
  CodeCanvas::code_pos_t current = populated_code_canvas->add_to_f_body(
    "%" + name + " = memref.get_global @" + name + " : memref<" + std::to_string(total_size) + "xi8>"
  );
  current = populated_code_canvas->add_to_f_body(
    "%" + name + "_" + name_field_1 + " = memref.subview %" + name + "[0][" + std::to_string(size_field_1) + "][1] : memref<" + std::to_string(total_size) + "xi8> to memref<" + std::to_string(size_field_1) + "xi8>"
  );
  current = populated_code_canvas->add_to_f_body(
    "%" + name + "_" + name_field_2 + " = memref.subview %" + name + "[" + std::to_string(size_field_1 + gap) + "][" + std::to_string(size_field_2) + "][1] : memref<" + std::to_string(total_size) + "xi8> to memref<" + std::to_string(size_field_2) + "xi8>"
  );
  
  populated_code_canvas->set_allocation_pos(it);
  populated_code_canvas->set_deallocation_pos(CodeCanvas::INVALID_CODE_POS);
  populated_code_canvas->set_lifetime_pos(current);
  return populated_code_canvas;
}
