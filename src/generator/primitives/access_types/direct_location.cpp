/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "direct_location.h"

#include <iostream>
#include <ostream>

#include "misc.h"
#include "generator/primitives/access_types/read_action.h"
#include "generator/primitives/bug_types/spatial/flow/flow.h"

std::vector<std::string> DirectLocation::generate(std::shared_ptr<AccessAction> action, const std::string &access_var_name, size_t size) const
{
  return generate_split(action, access_var_name, size).to_lines();
}

AccessLocation::SplitAccessLines DirectLocation::generate_split(
  std::shared_ptr<AccessAction> action,
  const std::string &access_var_name,
  size_t size
) const
{
  SplitAccessLines split_lines;
  if (is_a<ReadAction>(action))
  {
    // READ
    split_lines.aux_variable = "volatile char read_value[" + std::to_string(size) + "];";
    split_lines.access_lines.push_back("for (ssize_t i = 0; i < " + std::to_string(size) + "; i++)");
    split_lines.access_lines.push_back("{");
    split_lines.access_lines.push_back("  read_value[i] = " + access_var_name + "[i];");
    split_lines.access_lines.push_back("}");
    split_lines.access_lines.emplace_back("_use(read_value);" );
  }
  else
  {
    // WRITE
    split_lines.aux_variable = "";
    split_lines.access_lines.push_back("for (ssize_t i = 0; i < " + std::to_string(size) + "; i++)");
    split_lines.access_lines.push_back("{");
    split_lines.access_lines.push_back("  " + access_var_name + "[i] = 0xFF;");
    split_lines.access_lines.push_back("}");

    split_lines.access_lines.emplace_back("_use(" + access_var_name + ");" );
  }
  return split_lines;
}


AccessLocation::SplitAccessLines DirectLocation::generate_split_known_size(
  std::shared_ptr<AccessAction> action,
  const std::string &access_var_name,
  size_t size
) const
{
  SplitAccessLines split_lines;
  if (is_a<ReadAction>(action))
  {
    // READ
    split_lines.aux_variable = "volatile char read_value[" + std::to_string(size) + "];";

    split_lines.access_lines.push_back("for (ssize_t i = 0; i < " + std::to_string(size) + "; i++)");
    split_lines.access_lines.push_back("{");
    split_lines.access_lines.push_back("  read_value[i] = " + access_var_name + "[i];");
    split_lines.access_lines.push_back("}");
    split_lines.access_lines.emplace_back("_use(read_value);" );
  }
  else
  {
    // WRITE
    split_lines.aux_variable = "";

    split_lines.access_lines.push_back("for (ssize_t i = 0; i < " + std::to_string(size) + "; i++)");
    split_lines.access_lines.push_back("{");
    split_lines.access_lines.push_back("  " + access_var_name + "[i] = 0xFF;");
    split_lines.access_lines.push_back("}");

    split_lines.access_lines.emplace_back("_use(" + access_var_name + ");" );
  }
  return split_lines;
}


std::vector<std::string> DirectLocation::generate_at_index(
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
      "volatile char read_value[" + std::to_string(size) + "];",
    };

    if (generate_preconditions_check_distance)
    {
      lines.push_back( "if ( !(" + generate_preconditions_check_distance(index) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);" );
    };

    lines.push_back("for (ssize_t access_index = 0; access_index < " + std::to_string(size) + "; access_index++)");
    lines.emplace_back("{");
    lines.push_back("  read_value[access_index] = " + access_var_name + "[access_index + " + index + "];");
    lines.emplace_back("}");
    lines.emplace_back("_use(read_value);" );
  }
  else
  {
    // WRITE
    if (generate_preconditions_check_distance)
    {
      lines = {
        "if ( !(" + generate_preconditions_check_distance(index) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);"
      };
    }
    else
    {
      lines = {};
    }
    lines.push_back("for (ssize_t i = 0; i < " + std::to_string(size) + "; i++)");
    lines.emplace_back("{");
    lines.push_back("  " + access_var_name + "[i + " + index + "] = 0xFF;");
    lines.emplace_back("}");
    lines.emplace_back("_use(" + access_var_name + ");" );
  }
  return lines;
}

std::vector<std::string> DirectLocation::generate_with_runtime_index(
  std::shared_ptr<AccessAction> action,
  const std::string &access_var_name,
  std::string index,
  std::string distance,
  std::function<std::string(const std::string&)> generate_preconditions_check_distance,
  std::function<std::string(const std::string&, const std::string&, const std::string&)> generate_preconditions_check_in_range
) const
{
  std::vector<std::string> lines;
  if (is_a<ReadAction>(action))
  {
    // READ
    lines = {
      "volatile char tmp;",
      "for (" + index + " = 0; " + index + " < " + distance + "; " + index + "++)",
      "{",
      "  tmp = " + access_var_name + "[" + index + "];",
      "}",
      "_use(&tmp);",
    };
    if (!distance.empty() && generate_preconditions_check_distance) lines.insert(lines.begin(), "if ( !(" + generate_preconditions_check_distance(distance) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);");
  }
  else
  {
    // WRITE
    lines = {
      "for (" + index + " = 0; " + index + " < " + distance + "; " + index + "++)",
      "{",
      "  " + access_var_name + "[" + index + "] = 0xFF;",
      "}",
      "_use(" + access_var_name + ");"
    };
    if (!distance.empty())
    {
      if (generate_preconditions_check_in_range) lines.insert(lines.begin(), "if ( " + generate_preconditions_check_in_range(index, "&" + access_var_name + "[0]", "&" + access_var_name + "[" + distance + "]") + " ) _exit(PRECONDITIONS_FAILED_VALUE);");
      if (generate_preconditions_check_distance) lines.insert(lines.begin(), "if ( !(" + generate_preconditions_check_distance(distance) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);");
    }
  }
  return lines;
}

AccessLocation::SplitAccessLines DirectLocation::generate_bulk_split(
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
  if (is_a<ReadAction>(action))
  {
    // READ
    // split_lines.aux_variable_type = "char *";
    split_lines.aux_variable = "aux_ptr";
    split_lines.result = "aux_ptr";
    split_lines.access_lines = { {
      "volatile char tmp;",
      "char *aux_ptr = &" + from + "[0];",
      "while( GET_ADDR_BITS(aux_ptr) != GET_ADDR_BITS(" + to + ") )",
      "{",
      "  tmp = *aux_ptr;",
      "  " + generate_counter_update("aux_ptr") + ";",
      "  _use(&tmp);",
      "}"
    } };
    if (!distance.empty() && generate_preconditions_check_distance)
    {
      split_lines.access_lines.insert(split_lines.access_lines.begin(), "if ( !(" + generate_preconditions_check_distance(distance) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);");
    }
  }
  else
  {
    // WRITE
    // the caller must handle the allocation of aux_ptr, as it might be overwritten
    split_lines.aux_variable_type = "char *";
    split_lines.aux_variable = "aux_ptr";
    split_lines.result = "aux_ptr";
    split_lines.aux_variable_needs_allocation = true;
    split_lines.access_lines = {
      "aux_ptr = " + from + ";",
      "while( GET_ADDR_BITS(aux_ptr) != GET_ADDR_BITS(" + to + ") )",
      "{",
      "  *aux_ptr = 0xFF;",
      "  " + generate_counter_update("aux_ptr") + ";",
      "  _use(aux_ptr);",
      "}",
    };
    if (!distance.empty())
    {
      if (generate_preconditions_check_in_range) split_lines.access_lines.insert(split_lines.access_lines.begin(), "if ( " + generate_preconditions_check_in_range("aux_ptr", from, to) + " ) _exit(PRECONDITIONS_FAILED_VALUE);");
      if (generate_preconditions_check_distance) split_lines.access_lines.insert(split_lines.access_lines.begin(), "if ( !(" + generate_preconditions_check_distance(distance) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);");
    }
  }
  return split_lines;
}

AccessLocation::SplitAccessLines DirectLocation::generate_bulk_split(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  ssize_t distance,
  std::function<std::string(const std::string&, const std::string&, const std::string&)> generate_preconditions_check_in_range,
  std::function<std::string(const std::string&)> generate_counter_update
) const
{
  SplitAccessLines split_lines;
  if (is_a<ReadAction>(action))
  {
    // READ
    split_lines.aux_variable = "reach_index";
    split_lines.result = "(" + from + " + reach_index)";
    split_lines.access_lines = { {
      "volatile char tmp;",
      "ssize_t reach_index = 0;",
      "while( GET_ADDR_BITS(&" + from + "[reach_index]) != GET_ADDR_BITS(" + to + ") )",
      "{",
      "  tmp = " + from + "[reach_index];",
      "  " + generate_counter_update("reach_index") + ";",
      "  _use(&tmp);",
      "}"
    } };
  }
  else
  {
    // WRITE
    // the caller must handle the allocation of aux_ptr, as it might be overwritten
    split_lines.aux_variable = "reach_index";
    split_lines.aux_variable_type = "ssize_t";
    split_lines.aux_variable_needs_allocation = true;
    split_lines.result = "(" + from + " + reach_index)";
    split_lines.access_lines = {
      "reach_index = 0;",
      "while( GET_ADDR_BITS(&" + from + "[reach_index]) != GET_ADDR_BITS(" + to + ") )",
      "{",
      "  " + from + "[reach_index] = 0xFF;",
      "  " + generate_counter_update("reach_index") + ";",
      "  _use(&" + from + "[reach_index]);",
      "}",
    };
  }
  return split_lines;
}


std::vector<std::string> DirectLocation::generate_uint32(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  std::string distance,
  size_t size,
  std::function<std::string(const std::string&)>  generate_preconditions_check_distance
) const
{
  std::vector<std::string> lines;
  if (is_a<ReadAction>(action))
  {
    // READ
    lines = {
      "volatile uint32_t read_value;",
      "read_value = *((volatile uint32_t *)(" + from + " + (" + std::to_string(size) + " - 3)));",
      "_use(&read_value);"
    };
    if (!distance.empty() && generate_preconditions_check_distance)
    {
      lines.insert(lines.begin(), "if ( !(" + distance + " < (" + std::to_string(size) + " + 3) ) ) _exit(PRECONDITIONS_FAILED_VALUE);");
      lines.insert(lines.begin(), "if ( !(" + generate_preconditions_check_distance(distance) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);");
    }
  }
  else
  {
    // WRITE
    lines = {
      "*((volatile uint32_t *)(" + from + " + (" + std::to_string(size) + " - 1))) = 0xFFFFFFFF;",
    };
    if (!distance.empty() && generate_preconditions_check_distance)
    {
      lines.insert(lines.begin(), "if ( !(" + distance + " < (" + std::to_string(size) + " + 3) ) ) _exit(PRECONDITIONS_FAILED_VALUE);");
      lines.insert(lines.begin(), "if ( !(" + generate_preconditions_check_distance(distance) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);");
    }
  }
  return lines;
}


std::vector<std::string> DirectLocation::generate_uint8(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  std::string distance,
  size_t size,
  std::function<std::string(const std::string&)>  generate_preconditions_check_distance
) const
{
  std::vector<std::string> lines;
  if (is_a<ReadAction>(action))
  {
    // READ
    lines = {
      "volatile uint8_t read_value;",
      "read_value = *((volatile uint8_t *)(" + from + " + (" + std::to_string(size) + " - 1)));",
      "_use(&read_value);"
    };
    if (!distance.empty() && generate_preconditions_check_distance)
    {
      lines.insert(lines.begin(), "if ( !(" + distance + " < (" + std::to_string(size) + " + 1) ) ) _exit(PRECONDITIONS_FAILED_VALUE);");
      lines.insert(lines.begin(), "if ( !(" + generate_preconditions_check_distance(distance) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);");
    }
  }
  else
  {
    // WRITE
    lines = {
      "*((volatile uint8_t *)(" + from + " + (" + std::to_string(size) + " - 1))) = 0xFF;",
    };
    if (!distance.empty() && generate_preconditions_check_distance)
    {
      lines.insert(lines.begin(), "if ( !(" + distance + " < (" + std::to_string(size) + " + 1) ) ) _exit(PRECONDITIONS_FAILED_VALUE);");
      lines.insert(lines.begin(), "if ( !(" + generate_preconditions_check_distance(distance) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);");
    }
  }
  return lines;
}