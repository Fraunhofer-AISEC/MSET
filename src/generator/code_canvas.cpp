/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "code_canvas.h"

#include <cassert>
#include <sstream>

CodeCanvas::CodeCanvas():
  number_of_globals(0),
  number_of_locals(0)
{
  code_lines = {
    "/*",                                                     // 0
    " * This file is distributed under the Apache License, Version 2.0; refer to", // 1
    " * LICENSE for details.",                                // 2
    " *",                                                     // 3
    " * Generated by MSET 1.0.",                              // 4
    " */",                                                    // 5
    "",                                                       // 6
    "#include <unistd.h> // _exit", // 7
    "#include <stdint.h> // SIZE_MAX", // 8
    "#include <stdlib.h>", // 9
    "#include <string.h>", // 10
    "",                    // 11
    "#ifdef ADDR_MASK",    // 12
    "#define GET_ADDR_BITS(p) ((size_t)(p) & ADDR_MASK)", // 13
    "#else",                                            // 14
    "#define GET_ADDR_BITS(p) ((size_t)(p) & (size_t)0xffffffffffffull)",             // 15
    "#endif",                                           // 16
    "", // 17
    "volatile void *_use(volatile void *p) { return p; }", // 18
    "const char content[8] = \"ZZZZZZZ\";", // 19
    "", // 20
    "// types",     // 21
    "",             // 22
    "// globals",   // 23
    "",             // 24
    "",             // 25
    "int f()",      // 26
    "{",            // 27
    "  // locals",  // 28
    "",             // 29
    "",             // 30
    "",             // 31
    "  return 0;",  // 32
    "}",            // 33
    "",             // 34
    "int main()",   // 35
    "{",            // 36
    "  f();",       // 37
    "",             // 38
    "  return 0;",  // 39
    "}"             // 40
  };

  types_pos         = 22;
  global_start_pos  = 24;
  global_pos        = 25;
  locals_start_pos  = 29;
  locals_end_pos    = 30;
  start_of_f_pos    = 28;
  f_call_pos        = 37;
  current_pos_in_f  = locals_end_pos + 1;
  end_of_f_pos      = current_pos_in_f + 1;
  current_pos_in_main = f_call_pos + 1;
  current_pos_in_other_f = INVALID_CODE_POS;
  other_f_call_pos = INVALID_CODE_POS;
}

CodeCanvas::code_pos_t CodeCanvas::add_type(const std::vector<std::string> &lines)
{
  assert( types_pos != INVALID_CODE_POS );
  assert( types_pos < code_lines.size() );
  code_lines.insert(code_lines.begin() + types_pos, lines.begin(), lines.end());
  _update_indexes(types_pos, lines.size());
  return types_pos;
}

CodeCanvas::code_pos_t CodeCanvas::add_global(const std::vector<std::string> &lines)
{
  assert( global_pos != INVALID_CODE_POS );
  assert( global_pos < code_lines.size() );
  code_lines.insert(code_lines.begin() + global_pos, lines.begin(), lines.end());
  _update_indexes(global_pos, lines.size());
  ++number_of_globals;
  return global_pos;

}

CodeCanvas::code_pos_t CodeCanvas::add_global(const std::string &line)
{
  return add_global(std::vector<std::string>{line});
}

CodeCanvas::code_pos_t CodeCanvas::add_global_first(const std::vector<std::string> &lines)
{
  assert( global_start_pos != INVALID_CODE_POS );
  assert( global_start_pos < code_lines.size() );
  code_lines.insert(code_lines.begin() + global_start_pos, lines.begin(), lines.end());
  _update_indexes(global_start_pos + 1, lines.size());
  ++number_of_globals;
  return global_start_pos;
}

CodeCanvas::code_pos_t CodeCanvas::add_global_first(const std::string &line)
{
  return add_global_first(std::vector<std::string>{line});
}

CodeCanvas::code_pos_t CodeCanvas::add_local(const std::vector<std::string> &lines)
{
  ++number_of_locals;
  return add_at(locals_end_pos, lines);
}

CodeCanvas::code_pos_t CodeCanvas::add_local(const std::string &line)
{
  ++number_of_locals;
  return add_at(locals_end_pos, std::vector<std::string>{line}, "  ");
}

CodeCanvas::code_pos_t CodeCanvas::add_local_first(const std::vector<std::string> &lines)
{
  assert( locals_start_pos != INVALID_CODE_POS );
  assert( locals_start_pos < code_lines.size() );
  ++number_of_locals;
  std::vector<std::string> indented_lines{lines.size()};
  for ( size_t i = 0; i < lines.size(); i++ )
  {
    indented_lines[i] = "  " + lines[i];
  }
  code_lines.insert(code_lines.begin() + locals_start_pos, indented_lines.begin(), indented_lines.end());
  _update_indexes(locals_start_pos + 1, lines.size());
  return locals_start_pos + lines.size();
}

CodeCanvas::code_pos_t CodeCanvas::add_local_first(const std::string &line)
{
  return add_local_first(std::vector<std::string>{line});
}


CodeCanvas::code_pos_t CodeCanvas::add_to_f_body(const std::vector<std::string> &lines)
{
  assert( current_pos_in_f != INVALID_CODE_POS );
  assert( current_pos_in_f < code_lines.size() );
  std::vector<std::string> indented_lines{lines.size()};
  for ( size_t i = 0; i < lines.size(); i++ )
  {
    indented_lines[i] = "  " + lines[i];
  }
  code_lines.insert(code_lines.begin() + current_pos_in_f, indented_lines.begin(), indented_lines.end());
  _update_indexes(current_pos_in_f, lines.size());
  return current_pos_in_f;
}

CodeCanvas::code_pos_t CodeCanvas::add_to_f_body(const std::string &line)
{
  return add_to_f_body(std::vector<std::string>{line});
}

CodeCanvas::code_pos_t CodeCanvas::add_to_f_body_end(const std::vector<std::string> &lines)
{
  assert( end_of_f_pos != INVALID_CODE_POS );
  assert( end_of_f_pos < code_lines.size() );
  std::vector<std::string> indented_lines{lines.size()};
  for ( size_t i = 0; i < lines.size(); i++ )
  {
    indented_lines[i] = "  " + lines[i];
  }
  code_lines.insert(code_lines.begin() + end_of_f_pos, indented_lines.begin(), indented_lines.end());
  _update_indexes(end_of_f_pos, lines.size());
  return end_of_f_pos;
}

CodeCanvas::code_pos_t CodeCanvas::add_to_f_body_end(const std::string &line)
{
  return add_to_f_body_end(std::vector<std::string>{line});
}

CodeCanvas::code_pos_t CodeCanvas::add_to_other_f_body(const std::vector<std::string> &lines)
{
  if ( current_pos_in_other_f == INVALID_CODE_POS )
  {
    _generate_other_f_and_call();
  }
  assert( current_pos_in_other_f < code_lines.size() );
  std::vector<std::string> indented_lines{lines.size()};
  for ( size_t i = 0; i < lines.size(); i++ )
  {
    indented_lines[i] = "  " + lines[i];
  }
  code_lines.insert(code_lines.begin() + current_pos_in_other_f, indented_lines.begin(), indented_lines.end());
  _update_indexes(current_pos_in_other_f, lines.size());
  return current_pos_in_other_f;
}

CodeCanvas::code_pos_t CodeCanvas::add_to_other_f_body(const std::string &line)
{
  return add_to_other_f_body(std::vector<std::string>{line});
}

CodeCanvas::code_pos_t CodeCanvas::add_to_main_body(const std::vector<std::string> &lines)
{
  if ( current_pos_in_main == INVALID_CODE_POS )
  {
    _generate_other_f_and_call();
  }
  assert( current_pos_in_main < code_lines.size() );
  std::vector<std::string> indented_lines{lines.size()};
  for ( size_t i = 0; i < lines.size(); i++ )
  {
    indented_lines[i] = "  " + lines[i];
  }
  code_lines.insert(code_lines.begin() + current_pos_in_main, indented_lines.begin(), indented_lines.end());
  _update_indexes(current_pos_in_main, lines.size());
  return current_pos_in_main;
}

CodeCanvas::code_pos_t CodeCanvas::add_to_main_body(const std::string &line)
{
  return add_to_main_body(std::vector<std::string>{line});
}

CodeCanvas::code_pos_t CodeCanvas::add_at(code_pos_t where, const std::vector<std::string> &lines, const std::string indent)
{
  assert( where != INVALID_CODE_POS );
  assert( where < code_lines.size() );
  std::vector<std::string> indented_lines{lines.size()};
  for ( size_t i = 0; i < lines.size(); i++ )
  {
    indented_lines[i] = indent + lines[i];
  }
  code_lines.insert(code_lines.begin() + where, indented_lines.begin(), indented_lines.end());
  _update_indexes(where, lines.size());
  return where + lines.size();
}

CodeCanvas::code_pos_t CodeCanvas::add_at(code_pos_t where, const std::string &line, const std::string indent)
{
  return add_at(where, std::vector<std::string>{line}, indent);
}

CodeCanvas::code_pos_t CodeCanvas::prefix_line_with(code_pos_t where, const std::string &what)
{
  assert( where != INVALID_CODE_POS );
  assert( where < code_lines.size() );
  code_lines[where] = what + code_lines[where];
  return where;
}

std::string CodeCanvas::to_string() const
{
  std::ostringstream result;
  for (const auto& str : code_lines)
  {
    result << str << "\n";
  }
  return result.str();
}

void CodeCanvas::_generate_other_f_and_call()
{
  std::vector<std::string> other_f_body = {
    "int other_f()", // -5
    "{", // -4
    "", // -3
    "  return 0;", // -2
    "}", // -1
  };
  current_pos_in_other_f = add_at(global_pos + 1, other_f_body ) - 3;
  other_f_call_pos = add_at(f_call_pos + 1, "other_f();", "  ") - 1;
}

void CodeCanvas::_update_indexes(code_pos_t from, size_t amount)
{
  auto all_positions =   {&f_call_pos, &start_of_f_pos, &end_of_f_pos, &current_pos_in_f, &global_start_pos, &global_pos,
    &current_pos_in_other_f, &current_pos_in_main, &other_f_call_pos, &locals_start_pos, &locals_end_pos, &types_pos};
  for (auto pos: all_positions)
  {
    if ( *pos != INVALID_CODE_POS && *pos >= from )
    {
      assert( ( *pos + amount ) < code_lines.size() );
      *pos += amount;
    }
  }
}
