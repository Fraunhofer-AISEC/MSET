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
  std::string definition = "char " + name + "[" + std::to_string(size) + "]";
  if (initialize)
  {
    definition += " = {";
    for (size_t i = 0; i < (size - 1); i++)
    {
      definition += "0xAA, ";
    }
    definition += "0xAA}";
  }
  definition += ";";
  auto it = populated_code_canvas->add_global(definition);
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
  bool initialize
) const
{
  assert(size_field_1); assert(size_field_2);
  std::shared_ptr<RegionCodeCanvas> populated_code_canvas = std::make_shared<RegionCodeCanvas>(*canvas, "sizeof(struct T)");
  populated_code_canvas->add_type({
    "struct T",
    "{",
    "  char " + name_field_1 + "[" + std::to_string(size_field_1) + "];",
    "  char " + name_field_2 + "[" + std::to_string(size_field_2) + "];",
    "};"
  });
  std::string definition = "struct T " + name;
  if (initialize)
  {
    definition += " = { {";
    for (size_t i = 0; i < (size_field_1 - 1); i++)
    {
      definition += "0xAA, ";
    }
    definition += "0xAA}, {";
    for (size_t i = 0; i < (size_field_2 - 1); i++)
    {
      definition += "0xBB, ";
    }
    definition += "0xBB} }";
  }
  definition += ";";
  auto it = populated_code_canvas->add_global(definition);
  populated_code_canvas->set_allocation_pos(it);
  populated_code_canvas->set_deallocation_pos(CodeCanvas::INVALID_CODE_POS);
  populated_code_canvas->set_lifetime_pos(populated_code_canvas->get_current_pos_in_f());
  return populated_code_canvas;
}
