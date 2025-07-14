/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "stdlib_location.h"

#include <cassert>

#include "misc.h"
#include "generator/primitives/access_types/read_action.h"

std::vector<std::string> StdlibLocation::generate(
  std::shared_ptr<AccessAction> action, const std::string &access_var_name, size_t size) const
{
  return generate_split_const_vars(action, access_var_name, size).to_lines();
}

AccessLocation::SplitAccess StdlibLocation::generate_split_aux_vars(
  std::shared_ptr<AccessAction> action, const std::string &access_var_name, size_t size) const
{
  SplitAccess split_access;
  std::vector<std::string> code;
  if (is_a<ReadAction>(action))
  {
    // READ
    split_access.aux_variables = {
      {"read_value", "volatile char", std::to_string(size)},
    };

    split_access.access_lines.push_back( "memcpy( (void *)read_value, (void *)" + access_var_name + ", " + std::to_string(size) + ");" );
    split_access.access_lines.emplace_back("_use( read_value );" );
    split_access.result = "&" + access_var_name + "[" + std::to_string(size) + "]";
  }
  else
  {
    // WRITE
    split_access.aux_variables = {
      {"size", "volatile size_t", "", std::to_string(size)},
    };
    split_access.access_lines.push_back( "memset( (void *)" + access_var_name + ", 0xFF, size);" );
    split_access.access_lines.emplace_back("_use(" + access_var_name + ");" );
    split_access.result = "&" + access_var_name + "[var_size]";
  }
  return split_access;
}

AccessLocation::SplitAccess StdlibLocation::generate_split_const_vars(
  std::shared_ptr<AccessAction> action, const std::string &access_var_name, size_t size) const
{
  SplitAccess split_access;
  std::vector<std::string> code;
  if (is_a<ReadAction>(action))
  {
    // READ
    split_access.aux_variables = {
      {"read_value", "volatile char", std::to_string(size)},
    };

    split_access.access_lines.push_back( "memcpy( (void *)read_value, " + access_var_name + ", " + std::to_string(size) + ");" );
    split_access.access_lines.emplace_back("_use( read_value );" );
    split_access.result = "&" + access_var_name + "[" + std::to_string(size) + "]";
  }
  else
  {
    // WRITE
    split_access.access_lines.push_back( "memset( " + access_var_name + ", 0xFF, " + std::to_string(size) + ");" );
    split_access.access_lines.emplace_back("_use(" + access_var_name + ");" );
    split_access.result = "&" + access_var_name + "[8]";
  }
  return split_access;
}

AccessLocation::SplitAccess StdlibLocation::generate_bulk_split(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  std::string distance,
  std::function<std::string(const std::string&)> generate_preconditions_check_distance,
  std::function<std::string(const std::string&, const std::string&, const std::string&)> generate_preconditions_check_in_range,
  std::function<std::string(const std::string&)> generate_counter_update
) const
{
  SplitAccess split_access;
  if ( distance.empty() )
  {
    distance = "(" + to + " - " + from + ")"; // can never underflow
  }
  if (is_a<ReadAction>(action))
  {
    // READ
    split_access.aux_variables = {
      {"i", "volatile ssize_t", "", "0"},
    };

    if (generate_preconditions_check_distance)
    {
      split_access.access_lines = { "if ( !(" + generate_preconditions_check_distance(distance) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);" };
    }
    split_access.access_lines.insert( split_access.access_lines.end(), {
      "while( i < " + distance + " )",
      "{",
      "  volatile char read_value[1024];",
      "  size_t step_distance = (" + distance + " > (1024 + i)) ? 1024 : " + distance + " - i;",
      "  memcpy((void *)read_value, &" + from + "[i], step_distance);",
      "  i += step_distance;",
      "  _use(&read_value);",
      "}",
    } );
    split_access.result = "&" + from + "[i]";
  }
  else
  {
    // WRITE
    split_access.aux_variables = {
      {"i", "volatile ssize_t", "", "0"}, // volatile ssize_t i = 0;
      {"step_distance", "volatile size_t"}, // volatile size_t step_distance;
    };
    if (generate_preconditions_check_distance)
    {
      split_access.access_lines = { "if ( !(" + generate_preconditions_check_distance(distance) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);" };
    }
    if (generate_preconditions_check_in_range)
    {
      split_access.access_lines.insert( split_access.access_lines.end(), {"if ( " + generate_preconditions_check_in_range("i", from, to) + " ) _exit(PRECONDITIONS_FAILED_VALUE);"});
      split_access.access_lines.insert( split_access.access_lines.end(), {"if ( " + generate_preconditions_check_in_range("step_distance", from, to) + " ) _exit(PRECONDITIONS_FAILED_VALUE);"});
    }
    split_access.access_lines.insert( split_access.access_lines.end(), {
      "i = 0;",
      "while( GET_ADDR_BITS(&" + from + "[i]) < GET_ADDR_BITS(" + to + ") )",
      "{",
      "  step_distance = (GET_ADDR_BITS(" + to + ") > (1024 + GET_ADDR_BITS(&" + from + "[i]))) ? 1024 : GET_ADDR_BITS(" + to + ") - GET_ADDR_BITS(&" + from + "[i]);",
      "  memset(&" + from + "[i], 0xFF, step_distance);",
      "  i += step_distance;",
      "  _use(&" + from + "[i]);",
      "}",
      "_use(" + from + ");"
    } );
    split_access.result = "&" + from + "[i]";
  }
  return split_access;
}

AccessLocation::SplitAccess StdlibLocation::generate_bulk_split_using_aux_ptr(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  std::string distance,
  std::function<std::string(const std::string&)>  generate_preconditions_check_distance,
  std::function<std::string(const std::string&, const std::string&, const std::string&)> generate_preconditions_check_in_range,
  std::function<std::string(const std::string&)> generate_counter_update
) const
{
  SplitAccess split_access;

  if (is_a<ReadAction>(action))
  {
    // READ
    split_access.aux_variables = {
      {"aux_ptr", "volatile char *", "", from},
      {"i", "volatile size_t", "", "0"},
    };
    split_access.access_lines = { {
      "while( i < " + distance + " )",
      "{",
      "  volatile char read_value[1024];",
      "  size_t step_distance = (" + distance + " > (1024 + i)) ? 1024 : " + distance + " - i;",
      "  memcpy((void *)read_value, aux_ptr, step_distance);",
      "  aux_ptr += step_distance;",
      "  i += step_distance;",
      "  _use(&read_value);",
      "}",
    } };
    split_access.result = "aux_ptr";
  }
  else
  {
    // WRITE
    split_access.aux_variables = {
      {"aux_ptr", "volatile char *"},
      {"step_distance", "volatile size_t"},
    };
    split_access.access_lines = {};
    if (generate_preconditions_check_in_range)
    {
      split_access.access_lines.push_back( "if ( " + generate_preconditions_check_in_range("aux_ptr", from, to) + " ) _exit(PRECONDITIONS_FAILED_VALUE);" );
      split_access.access_lines.push_back( "if ( " + generate_preconditions_check_in_range("step_distance", from, to) + " ) _exit(PRECONDITIONS_FAILED_VALUE);" );
    }
    split_access.access_lines.insert( split_access.access_lines.end(), {
      "aux_ptr = " + from + ";",
      "while( GET_ADDR_BITS(aux_ptr) < GET_ADDR_BITS(" + to + ") )",
      "{",
      "  step_distance = (GET_ADDR_BITS(" + to + ") > (1024 + GET_ADDR_BITS(aux_ptr))) ? 1024 : GET_ADDR_BITS(" + to + ") - GET_ADDR_BITS(aux_ptr);",
      "  memset(aux_ptr, 0xFF, step_distance);",
      "  aux_ptr += step_distance;",
      "  _use(aux_ptr);",
      "}",
      "_use(" + from + ");"
    } );
    split_access.result = "aux_ptr";
  }

  return split_access;
}


std::vector<std::string> StdlibLocation::generate_at_index(
  std::shared_ptr<AccessAction> action,
  const std::string &access_var_name,
  std::string index,
  size_t size,
  std::function<std::string(const std::string&)> generate_preconditions_check_distance
) const
{
  std::vector<std::string> lines;
  if (is_a<ReadAction>(action))
  {
    // READ
    lines = {
      "char read_value[" + std::to_string(size) + "];",
    };
    if (generate_preconditions_check_distance)
    {
      lines.push_back( "if ( !(" + generate_preconditions_check_distance(index) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);" );
    }
    lines.insert( lines.end(), {
      "memcpy(read_value, &" + access_var_name + "[" + index + "], " + std::to_string(size) + ");",
      "_use(read_value);"
    } );
  }
  else
  {
    // WRITE
    for (size_t i = 0; i < size; i++)
    {
      assert(size > 0);
      lines = {};
      if (generate_preconditions_check_distance)
      {
        lines.push_back( "if ( !(" + generate_preconditions_check_distance(index) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);" );
      }
      lines.insert( lines.end(), {
        "memset((void *)&" + access_var_name + "[" + index + "], 0xFF, " + std::to_string(size) + ");",
        "_use(" + access_var_name + ");"
      });
    }
  }
  return lines;
}


std::vector<std::string> StdlibLocation::generate_using_runtime_index(
  std::shared_ptr<AccessAction> action,
  const std::string &access_var_name,
  std::string index,
  std::string distance,
  std::function<std::string(const std::string&)> generate_preconditions_check_distance,
  std::function<std::string(const std::string&, const std::string&, const std::string&)> generate_preconditions_check_in_range
) const
{
  assert(0 && "generate_using_runtime_index not used for StdlibLocation");
  return {};
}

std::vector<std::string> StdlibLocation::generate_uint32(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  std::string distance,
  size_t size,
  std::function<std::string(const std::string&)>  generate_preconditions_check_distance
) const
{
  assert(0 && "generate_uint32 not used for StdlibLocation");
  return {};
}

std::vector<std::string> StdlibLocation::generate_uint8(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  std::string distance,
  size_t size,
  std::function<std::string(const std::string&)>  generate_preconditions_check_distance
) const
{
  assert(0 && "generate_uint8 not used for StdlibLocation");
  return {};
}