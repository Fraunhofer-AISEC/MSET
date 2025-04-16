/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "origin_target_relation.h"

#include <cassert>

CodeCanvas::code_pos_t OriginTargetCodeCanvas::add_during_lifetime(const std::vector<std::string> &lines)
{
  // code_canvas.code_lines.insert(code_canvas.code_lines.begin() + lifetime_pos, lines.begin(), lines.end());
  add_at( lifetime_pos, lines, "  " );
  return lifetime_pos;
}

CodeCanvas::code_pos_t OriginTargetCodeCanvas::add_during_lifetime(const std::string &line)
{
  return add_during_lifetime(std::vector<std::string>{line});
}

void OriginTargetCodeCanvas::_update_indexes(CodeCanvas::code_pos_t from, size_t amount)
{
  CodeCanvas::_update_indexes(from, amount);
  auto all_iterators =   {&lifetime_pos};
  // code_canvas.update_indexes(from, amount);
  for (auto it: all_iterators)
  {
    if ( *it != CodeCanvas::INVALID_CODE_POS && *it >= from )
    {
      assert( *it < (code_lines.size() + amount) );
      *it += amount;
    }
  }
}