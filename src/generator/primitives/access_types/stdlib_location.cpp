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
  return generate_split(action, access_var_name, size).to_lines();
}

AccessLocation::SplitAccessLines StdlibLocation::generate_split(
  std::shared_ptr<AccessAction> action, const std::string &access_var_name, size_t size) const
{
  SplitAccessLines split_lines;
  std::vector<std::string> code;
  if (is_a<ReadAction>(action))
  {
    // READ
    split_lines.aux_variable = "char read_value[" + std::to_string(size) + "];";
    split_lines.access_lines.push_back( "memcpy( read_value, " + access_var_name + ", " + std::to_string(size) + ");" );
    split_lines.access_lines.emplace_back("_use( read_value );" );
    split_lines.result = "&" + access_var_name + "[" + std::to_string(size) + "]";
  }
  else
  {
    // WRITE

    // std::string content = "\"";
    // for (size_t j = 0; j < size - 1; j++)
    // {
    //   content += "Z";
    // }
    // content += "\"";
    split_lines.aux_variable = "static int var_size = " + std::to_string(size) + ";";
    split_lines.access_lines.push_back( "memcpy( " + access_var_name + ", content, var_size);" );
    // split_lines.access_lines.push_back( "memset( " + access_var_name + ", 0xFF, " + std::to_string(size) + ");" );
    split_lines.access_lines.emplace_back("_use(" + access_var_name + ");" );
    split_lines.result = "&" + access_var_name + "[var_size]";
  }
  return split_lines;
}

AccessLocation::SplitAccessLines StdlibLocation::generate_split_known_size(
  std::shared_ptr<AccessAction> action, const std::string &access_var_name, size_t size) const
{
  SplitAccessLines split_lines;
  std::vector<std::string> code;
  if (is_a<ReadAction>(action))
  {
    // READ
    split_lines.aux_variable = "char read_value[" + std::to_string(size) + "];";
    split_lines.access_lines.push_back( "memcpy( read_value, " + access_var_name + ", " + std::to_string(size) + ");" );
    split_lines.access_lines.emplace_back("_use( read_value );" );
    split_lines.result = "&" + access_var_name + "[" + std::to_string(size) + "]";
  }
  else
  {
    // WRITE

    split_lines.access_lines.push_back( "memset( " + access_var_name + ", 0xFF, " + std::to_string(size) + ");" );
    split_lines.access_lines.emplace_back("_use(" + access_var_name + ");" );
    split_lines.result = "&" + access_var_name + "[8]";
  }
  return split_lines;
}

AccessLocation::SplitAccessLines StdlibLocation::generate_bulk_split(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  std::string distance,
  std::function<std::string(const std::string&)> generate_preconditions_check_distance,
  std::function<std::string(const std::string&, const std::string&, const std::string&)> generate_preconditions_check_in_range,
  std::function<std::string(const std::string&)> generate_counter_update
) const
{
  SplitAccessLines split_lines;
  if ( distance.empty() )
  {
    distance = "(" + to + " - " + from + ")"; // can never underflow
  }
  if (is_a<ReadAction>(action))
  {
    // READ
    split_lines.aux_variable = "i";
    if (generate_preconditions_check_distance)
    {
      split_lines.access_lines = { "if ( !(" + generate_preconditions_check_distance(distance) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);" };
    }
    split_lines.access_lines.insert( split_lines.access_lines.end(), {
      // "size_t oob_distance = (size_t)" + to + " - (size_t)" + from + ";",
      "ssize_t i = 0;",
      "char tmp[1024];",
      "while( i < " + distance + " )",
      "{",
      "  size_t step_distance = (" + distance + " > (1024 + i)) ? 1024 : " + distance + " - i;",
      "  memcpy((void *)tmp, &" + from + "[i], step_distance);",
      "  i += step_distance;",
      "  _use(&tmp);",
      "}",
    } );
    split_lines.result = "&" + from + "[i]";
  }
  else
  {
    // WRITE
    split_lines.aux_variable = "i";
    split_lines.aux_variable_type = "ssize_t";
    split_lines.aux_variable_needs_allocation = true;
    if (generate_preconditions_check_distance)
    {
      split_lines.access_lines = { "if ( !(" + generate_preconditions_check_distance(distance) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);" };
    }
    if (generate_preconditions_check_in_range)
    {
      split_lines.access_lines.insert( split_lines.access_lines.end(), {"if ( " + generate_preconditions_check_in_range("i", from, to) + " ) _exit(PRECONDITIONS_FAILED_VALUE);"});
    }
    split_lines.access_lines.insert( split_lines.access_lines.end(), {
      "i = 0;",
      "while( GET_ADDR_BITS(&" + from + "[i]) < GET_ADDR_BITS(" + to + ") )",
      "{",
      "  size_t step_distance = (GET_ADDR_BITS(" + to + ") > (1024 + GET_ADDR_BITS(&" + from + "[i]))) ? 1024 : GET_ADDR_BITS(" + to + ") - GET_ADDR_BITS(&" + from + "[i]);",
      "  memset(&" + from + "[i], 0xFF, step_distance);",
      "  i += step_distance;",
      "  _use(&" + from + "[i]);",
      "}",
      "_use(" + from + ");"
    } );
    split_lines.result = "&" + from + "[i]";
  }
  return split_lines;
}

AccessLocation::SplitAccessLines StdlibLocation::generate_bulk_split(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  ssize_t distance,
  std::function<std::string(const std::string&, const std::string&, const std::string&)> generate_preconditions_check_in_range,
  std::function<std::string(const std::string&)> generate_counter_update
) const
{
  SplitAccessLines split_lines;
  assert(distance >= 0 );
  distance = std::abs(distance);
  if ( distance > 1024 )
  {
    if (is_a<ReadAction>(action))
    {
      // READ
      split_lines.aux_variable = "i";
      split_lines.access_lines = { {
        // "size_t oob_distance = (size_t)" + to + " - (size_t)" + from + ";",
        "ssize_t i = 0;",
        "char tmp[1024];",
        "while( i < " + std::to_string(distance) + " )",
        "{",
        "  size_t step_distance = (" + std::to_string(distance) + " > (1024 + i)) ? 1024 : " + std::to_string(distance) + " - i;",
        "  memcpy((void *)tmp, &" + from + "[i], step_distance);",
        "  i += step_distance;",
        "  _use(&tmp);",
        "}",
      } };
      split_lines.result = "&" + from + "[i]";
    }
    else
    {
      // WRITE
      split_lines.aux_variable = "i";
      split_lines.aux_variable_type = "ssize_t";
      split_lines.aux_variable_needs_allocation = true;
      split_lines.access_lines = {};
      if (generate_preconditions_check_in_range)
      {
        split_lines.access_lines.push_back( "if ( " + generate_preconditions_check_in_range("i", from, to) + " ) _exit(PRECONDITIONS_FAILED_VALUE);" );
      }
      split_lines.access_lines.insert( split_lines.access_lines.end(), {
        "i = 0;",
        "while( GET_ADDR_BITS(&" + from + "[i]) < GET_ADDR_BITS(" + to + ") )",
        "{",
        "  size_t step_distance = (GET_ADDR_BITS(" + to + ") > (1024 + GET_ADDR_BITS(&" + from + "[i]))) ? 1024 : GET_ADDR_BITS(" + to + ") - GET_ADDR_BITS(&" + from + "[i]);",
        "  memset(&" + from + "[i], 0xFF, step_distance);",
        "  i += step_distance;",
        "  _use(&" + from + "[i]);",
        "}",
        "_use(" + from + ");"
      } );
      split_lines.result = "&" + from + "[i]";
    }
  }
  else
  {
    // small distance

    if (is_a<ReadAction>(action))
    {
      std::string tmp_array_size;
      if ( distance == 0 ) tmp_array_size = "1"; // at least one byte
      else tmp_array_size = std::to_string(distance);
      // READ
      split_lines.aux_variable = "i";
      split_lines.access_lines = { {
        "char tmp[" + tmp_array_size + "];",
        "memcpy((void *)tmp, &" + from + "[0], " + std::to_string(distance) + ");",
        "_use(&tmp);",
        "ssize_t i = " + std::to_string(distance) + ";"
      } };
      split_lines.result = "&" + from + "[i]";
    }
    else
    {
      // WRITE

      // std::string content = "\"";
      // for (size_t j = 0; j < static_cast<size_t>(distance) - 1; j++)
      // {
      //   content += "Z";
      // }
      // content += "\"";

      split_lines.aux_variable = "i";
      split_lines.aux_variable_type = "ssize_t";
      split_lines.aux_variable_needs_allocation = true;
      split_lines.access_lines = {};
      if (generate_preconditions_check_in_range)
      {
        split_lines.access_lines.push_back( "if ( " + generate_preconditions_check_in_range("i", from, to) + " ) _exit(PRECONDITIONS_FAILED_VALUE);" );
      }
      split_lines.access_lines.insert( split_lines.access_lines.end(), {
        "i = 0;",
        // "memset(" + from + ", 0xFF, " + std::to_string(distance) + ");",
        "memcpy(" + from + ", content, " + std::to_string(distance) + ");",
        "_use(" + from + ");",
        "i = " + std::to_string(distance) + ";"
      } );
      split_lines.result = "&" + from + "[i]";
    }
  }
  return split_lines;
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
      // std::string content = "\"";
      // for (size_t j = 0; j < size - 1; j++)
      // {
      //   content += "Z";
      // }
      // content += "\"";
      lines = {};
      if (generate_preconditions_check_distance)
      {
        lines.push_back( "if ( !(" + generate_preconditions_check_distance(index) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);" );
      }
      lines.insert( lines.end(), {
        // "memset(&" + access_var_name + "[" + index + "], 0xFF, " + std::to_string(size) + ");",
        "memcpy(&" + access_var_name + "[" + index + "], content, " + std::to_string(size) + ");",
        "_use(" + access_var_name + ");"
      });
    }
  }
  return lines;
}


std::vector<std::string> StdlibLocation::generate_with_runtime_index(
  std::shared_ptr<AccessAction> action,
  const std::string &access_var_name,
  std::string index,
  std::string distance,
  std::function<std::string(const std::string&)> generate_preconditions_check_distance,
  std::function<std::string(const std::string&, const std::string&, const std::string&)> generate_preconditions_check_in_range
) const
{
  assert(0 && "generate_with_runtime_index not used for StdlibLocation");
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