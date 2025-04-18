/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include <iostream>
#include <cstdint>
#include <string>
#include <vector>

class RegionCodeCanvas;

class CodeCanvas
{
public:
  CodeCanvas(const CodeCanvas &other)
    : code_lines(other.code_lines),
      types_pos(other.types_pos),
      global_start_pos(other.global_start_pos),
      global_pos(other.global_pos),
      f_call_pos(other.f_call_pos),
      start_of_f_pos(other.start_of_f_pos),
      end_of_f_pos(other.end_of_f_pos),
      locals_start_pos(other.locals_start_pos),
      locals_end_pos(other.locals_end_pos),
      current_pos_in_f(other.current_pos_in_f),
      current_pos_in_main(other.current_pos_in_main),
      other_f_call_pos(other.other_f_call_pos),
      current_pos_in_other_f(other.current_pos_in_other_f),
      number_of_globals(other.number_of_globals),
      number_of_locals(other.number_of_locals)
  {
  }

  CodeCanvas(CodeCanvas &&other) noexcept
    : code_lines(std::move(other.code_lines)),
      types_pos(other.types_pos),
      global_start_pos(other.global_start_pos),
      global_pos(other.global_pos),
      f_call_pos(other.f_call_pos),
      start_of_f_pos(other.start_of_f_pos),
      end_of_f_pos(other.end_of_f_pos),
      locals_start_pos(other.locals_start_pos),
      locals_end_pos(other.locals_end_pos),
      current_pos_in_f(other.current_pos_in_f),
      current_pos_in_main(other.current_pos_in_main),
      other_f_call_pos(other.other_f_call_pos),
      current_pos_in_other_f(other.current_pos_in_other_f),
      number_of_globals(other.number_of_globals),
      number_of_locals(other.number_of_locals)
  {
  }

  CodeCanvas & operator=(const CodeCanvas &other)
  {
    if (this == &other)
      return *this;
    code_lines = other.code_lines;
    types_pos = other.types_pos;
    global_start_pos = other.global_start_pos;
    global_pos = other.global_pos;
    f_call_pos = other.f_call_pos;
    start_of_f_pos = other.start_of_f_pos;
    end_of_f_pos = other.end_of_f_pos;
    locals_start_pos = other.locals_start_pos;
    locals_end_pos = other.locals_end_pos;
    current_pos_in_f = other.current_pos_in_f;
    current_pos_in_main = other.current_pos_in_main;
    other_f_call_pos = other.other_f_call_pos;
    current_pos_in_other_f = other.current_pos_in_other_f;
    number_of_globals = other.number_of_globals;
    number_of_locals = other.number_of_locals;
    return *this;
  }

  CodeCanvas & operator=(CodeCanvas &&other) noexcept
  {
    if (this == &other)
      return *this;
    code_lines = std::move(other.code_lines);
    types_pos = other.types_pos;
    global_start_pos = other.global_start_pos;
    global_pos = other.global_pos;
    f_call_pos = other.f_call_pos;
    start_of_f_pos = other.start_of_f_pos;
    end_of_f_pos = other.end_of_f_pos;
    locals_start_pos = other.locals_start_pos;
    locals_end_pos = other.locals_end_pos;
    current_pos_in_f = other.current_pos_in_f;
    current_pos_in_main = other.current_pos_in_main;
    other_f_call_pos = other.other_f_call_pos;
    current_pos_in_other_f = other.current_pos_in_other_f;
    number_of_globals = other.number_of_globals;
    number_of_locals = other.number_of_locals;
    return *this;
  }

  using code_pos_t = size_t;
  static const size_t INVALID_CODE_POS = SIZE_MAX;

  CodeCanvas();
  virtual ~CodeCanvas() = default;

  code_pos_t add_type(const std::vector<std::string> &lines);
  code_pos_t add_global(const std::vector<std::string> &lines);
  code_pos_t add_global(const std::string &line);
  code_pos_t add_global_first(const std::vector<std::string> &lines);
  code_pos_t add_global_first(const std::string &line);
  code_pos_t add_local(const std::vector<std::string> &lines);
  code_pos_t add_local(const std::string &line);
  code_pos_t add_local_first(const std::vector<std::string> &lines);
  code_pos_t add_local_first(const std::string &line);

  code_pos_t add_to_f_body(const std::vector<std::string> &lines);
  code_pos_t add_to_f_body(const std::string &line);
  code_pos_t add_to_f_body_end(const std::vector<std::string> &lines);
  code_pos_t add_to_f_body_end(const std::string &line);
  code_pos_t add_to_other_f_body(const std::vector<std::string> &lines);
  code_pos_t add_to_other_f_body(const std::string &line);
  code_pos_t add_to_main_body(const std::vector<std::string> &lines);
  code_pos_t add_to_main_body(const std::string &line);
  code_pos_t add_at(code_pos_t where, const std::vector<std::string> &lines, const std::string indent = "");
  code_pos_t add_at(code_pos_t where, const std::string &line, const std::string indent = "");

  code_pos_t prefix_line_with(code_pos_t where, const std::string &what);

  code_pos_t get_f_call_pos() const { return f_call_pos; }
  code_pos_t get_current_pos_in_f() const { return current_pos_in_f; }
  code_pos_t get_other_f_call_pos() const { return other_f_call_pos; }

  std::string to_string() const;
  std::vector<std::string> get_lines() const { return code_lines; }

  int get_number_of_globals() const
  {
    return number_of_globals;
  }

  int get_number_of_locals() const
  {
    return number_of_locals;
  }
protected:
  void _generate_other_f_and_call();
  virtual void _update_indexes(code_pos_t from, size_t amount);
  std::vector<std::string> code_lines;
  code_pos_t types_pos;
  code_pos_t global_start_pos;
  code_pos_t global_pos;
  code_pos_t f_call_pos;
  code_pos_t start_of_f_pos;
  code_pos_t end_of_f_pos;
  code_pos_t locals_start_pos;
  code_pos_t locals_end_pos;
  code_pos_t current_pos_in_f;
  code_pos_t current_pos_in_main;
  code_pos_t other_f_call_pos;
  code_pos_t current_pos_in_other_f;

  int number_of_globals;
  int number_of_locals;
};
