/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "region.h"

#include <cassert>

RegionCodeCanvas::RegionCodeCanvas(const CodeCanvas &code_canvas, std::string var_size):
  CodeCanvas( code_canvas ),
  allocation_pos( CodeCanvas::INVALID_CODE_POS ),
  deallocation_pos( CodeCanvas::INVALID_CODE_POS ),
  lifetime_pos( CodeCanvas::INVALID_CODE_POS ),
  var_size(var_size),
  static_var_size(-1)
{
}

RegionCodeCanvas::RegionCodeCanvas(const CodeCanvas &code_canvas, size_t static_var_size):
  CodeCanvas( code_canvas ),
  allocation_pos( CodeCanvas::INVALID_CODE_POS ),
  deallocation_pos( CodeCanvas::INVALID_CODE_POS ),
  lifetime_pos( CodeCanvas::INVALID_CODE_POS ),
  static_var_size(static_var_size)
{
}

CodeCanvas::code_pos_t RegionCodeCanvas::add_during_lifetime(const std::vector<std::string> &lines)
{
  add_at( lifetime_pos, lines, "  " );
  return lifetime_pos;
}

void RegionCodeCanvas::_update_indexes(CodeCanvas::code_pos_t from, size_t amount)
{
  CodeCanvas::_update_indexes(from, amount);
  auto all_iterators =   {&allocation_pos, &deallocation_pos, &lifetime_pos};
  for (auto it: all_iterators)
  {
    if ( *it != CodeCanvas::INVALID_CODE_POS && *it >= from )
    {
      assert( *it < (code_lines.size() + amount) );
      *it += amount;
    }
  }
}

CodeCanvas::code_pos_t RegionCodeCanvas::add_during_lifetime(const std::string &line)
{
  return add_during_lifetime(std::vector<std::string>{line});
}


Region::Region(std::string name):
  Property(std::move(name))
{
}

