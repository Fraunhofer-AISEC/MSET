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
  CodeCanvas::code_pos_t allocation_pos = populated_code_canvas->add_local("char " + name + "[" + std::to_string(size) + "] = \"\";");
  CodeCanvas::code_pos_t current = allocation_pos;
  if (initialize)
  {
    for (size_t i = 0; i < size; i++)
    {
      current = populated_code_canvas->add_to_f_body(name + "[" + std::to_string(i) + "] = 0xAA;");
    }
  }
  populated_code_canvas->set_allocation_pos(allocation_pos - 1);
  populated_code_canvas->set_deallocation_pos(populated_code_canvas->get_f_call_pos() + 1);
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
  CodeCanvas::code_pos_t allocation_pos = populated_code_canvas->add_to_other_f_body("char " + name + "[" + std::to_string(size) + "];");
  CodeCanvas::code_pos_t current = allocation_pos;
  if (initialize)
  {
    for (size_t i = 0; i < size; i++)
    {
      current = populated_code_canvas->add_to_other_f_body(name + "[" + std::to_string(i) + "] = 0xAA;");
    }
  }
  populated_code_canvas->set_allocation_pos(allocation_pos - 1);
  populated_code_canvas->set_deallocation_pos(populated_code_canvas->get_other_f_call_pos() + 1);
  populated_code_canvas->set_lifetime_pos(current);

  return populated_code_canvas;

}

std::shared_ptr<RegionCodeCanvas> StackRegion::generate_array(std::shared_ptr<CodeCanvas> canvas, std::string name, size_t size, size_t array_size,
  bool initialize) const
{
  std::shared_ptr<RegionCodeCanvas> populated_code_canvas = std::make_shared<RegionCodeCanvas>(*canvas, size);
  CodeCanvas::code_pos_t allocation_pos = populated_code_canvas->add_to_f_body("char " + name + "[" + std::to_string(array_size) + "][" + std::to_string(size) + "];");
  CodeCanvas::code_pos_t current = allocation_pos;
  if (initialize)
  {
    populated_code_canvas->add_to_f_body("for (int i = 0; i < " + std::to_string(array_size) + "; i++)");
    populated_code_canvas->add_to_f_body("{");
    for (size_t i = 0; i < size; i++)
    {
      populated_code_canvas->add_to_f_body("  " + name + "[i][" + std::to_string(i) + "] = 0xAA;");
    }
    current = populated_code_canvas->add_to_f_body("}");
  }
  populated_code_canvas->set_allocation_pos(allocation_pos - 1);
  populated_code_canvas->set_deallocation_pos(populated_code_canvas->get_other_f_call_pos() + 1);
  populated_code_canvas->set_lifetime_pos(current);

  return populated_code_canvas;

}

std::shared_ptr<RegionCodeCanvas> StackRegion::generate(
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
  CodeCanvas::code_pos_t allocation_pos = populated_code_canvas->add_local("struct T " + name + ";");
  CodeCanvas::code_pos_t current = allocation_pos;
  if (initialize)
  {
    for (size_t i = 0; i < size_field_1; i++)
    {
      current = populated_code_canvas->add_to_f_body(name + "." + name_field_1 + "[" + std::to_string(i) + "] = 0xAA;");
    }
    for (size_t i = 0; i < size_field_2; i++)
    {
      current = populated_code_canvas->add_to_f_body(name + "." + name_field_2 + "[" + std::to_string(i) + "] = 0xBB;");
    }
  }
  populated_code_canvas->set_allocation_pos(allocation_pos - 1);
  populated_code_canvas->set_deallocation_pos(populated_code_canvas->get_f_call_pos() + 1);
  populated_code_canvas->set_lifetime_pos(current);

  return populated_code_canvas;
}
