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
    "char *" + name + " = (char *)malloc( " + std::to_string(size) + " );",
    "  "
  );
  CodeCanvas::code_pos_t allocation_pos = where;

  CodeCanvas::code_pos_t lifetime_pos;
  if (initialize)
  {
    for (size_t i = 0; i < size; i++)
    {
      where = populated_code_canvas->add_at(where, name + "[" + std::to_string(i) + "] = 0xAA;", "  ");
    }
    lifetime_pos = where;
  }
  else
  {
    lifetime_pos = populated_code_canvas->add_at(where, "");
  }
  CodeCanvas::code_pos_t deallocation_pos = populated_code_canvas->add_to_f_body_end("free(" + name + ");");
  populated_code_canvas->set_allocation_pos(allocation_pos);
  populated_code_canvas->set_deallocation_pos(deallocation_pos);
  populated_code_canvas->set_lifetime_pos(lifetime_pos);
  return populated_code_canvas;
}

std::shared_ptr<RegionCodeCanvas> HeapRegion::generate(std::shared_ptr<CodeCanvas> canvas, std::string name, size_t size, bool initialize) const
{
  std::shared_ptr<RegionCodeCanvas> populated_code_canvas = std::make_shared<RegionCodeCanvas>(*canvas, size);

  CodeCanvas::code_pos_t allocation_pos = populated_code_canvas->add_to_f_body(
    "char *" + name + " = (char *)malloc( " + std::to_string(size) + " );"
  );

  CodeCanvas::code_pos_t lifetime_pos = populated_code_canvas->get_lifetime_pos();
  if (initialize)
  {
    for (size_t i = 0; i < size; i++)
    {
      lifetime_pos = populated_code_canvas->add_to_f_body(name + "[" + std::to_string(i) + "] = 0xAA;");
    }
  }
  else
  {
    lifetime_pos = populated_code_canvas->add_to_f_body("");
  }
  CodeCanvas::code_pos_t deallocation_pos = populated_code_canvas->add_to_f_body_end("free(" + name + ");");
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
  CodeCanvas::code_pos_t allocation_pos = populated_code_canvas->add_to_f_body(
    "struct T *" + name + " = (struct T *)malloc( sizeof(struct T) );"
  );

  CodeCanvas::code_pos_t lifetime_pos = populated_code_canvas->get_lifetime_pos();
  if (initialize)
  {
    for (size_t i = 0; i < size_field_1; i++)
    {
      lifetime_pos = populated_code_canvas->add_to_f_body(name + "->" + name_field_1 + "[" + std::to_string(i) + "] = 0xAA;");
    }
    for (size_t i = 0; i < size_field_2; i++)
    {
      lifetime_pos = populated_code_canvas->add_to_f_body(name + "->" + name_field_2 + "[" + std::to_string(i) + "] = 0xBB;");
    }
  }
  else
  {
    lifetime_pos = populated_code_canvas->add_to_f_body("");
  }
  CodeCanvas::code_pos_t deallocation_pos = populated_code_canvas->add_to_f_body_end("free(" + name + ");");
  populated_code_canvas->set_allocation_pos(allocation_pos);
  populated_code_canvas->set_deallocation_pos(deallocation_pos);
  populated_code_canvas->set_lifetime_pos(lifetime_pos);
  return populated_code_canvas;
}


std::vector<std::string> HeapRegion::generate_reallocation(std::string name, size_t size, bool initialize, std::string indent) const
{
  std::vector<std::string> reallocation = {indent + name + " = (char *)malloc( " + std::to_string(size) + " );"};

  if (initialize)
  {
    for (size_t i = 0; i < size; i++)
    {
      reallocation.push_back(indent + name + "[" + std::to_string(i) + "] = 0xAA;");
    }
  }
  return reallocation;
}

std::vector<std::string> HeapRegion::generate_deallocation(std::string name, std::string indent) const
{
  return {indent + "free(" + name + ");"};
}
