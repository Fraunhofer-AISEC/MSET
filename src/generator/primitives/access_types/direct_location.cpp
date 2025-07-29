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

// simple generate
std::vector<std::string> DirectLocation::generate(std::shared_ptr<AccessAction> action, const std::string &access_var_name, size_t size) const
{
  return generate_split_const_vars(action, access_var_name, size).to_lines();
}

// simple split, using auxiliary size and content variables
AccessLocation::SplitAccess DirectLocation::generate_split_aux_vars(
  std::shared_ptr<AccessAction> action,
  const std::string &access_var_name,
  size_t size
) const
{
  SplitAccess split_access;
  if (is_a<ReadAction>(action))
  {
    // READ
    split_access.aux_variables = {
      {"read_value", "volatile char", std::to_string(size), ""},
      {"i", "volatile size_t"},
      {"size", "volatile size_t", "", std::to_string(size)},
    };

    split_access.access_lines.emplace_back("for (i = 0; i < size; i++)");
    split_access.access_lines.emplace_back("{");
    split_access.access_lines.push_back("  read_value[i] = " + access_var_name + "[i];");
    split_access.access_lines.emplace_back("}");
    split_access.access_lines.emplace_back("_use(read_value);" );
  }
  else
  {
    // WRITE
    split_access.aux_variables = {
      {"i", "volatile size_t", ""},
      {"size", "volatile size_t", "", std::to_string(size)},
    };
    split_access.access_lines.emplace_back("for (i = 0; i < size; i++)");
    split_access.access_lines.emplace_back("{");
    split_access.access_lines.push_back("  " + access_var_name + "[i] = content[i];");
    split_access.access_lines.emplace_back("}");

    split_access.access_lines.emplace_back("_use(" + access_var_name + ");" );
  }
  split_access.description = "auxiliary variables";
  return split_access;
}

// simple split, using const size and content variables
AccessLocation::SplitAccess DirectLocation::generate_split_const_vars(
  std::shared_ptr<AccessAction> action,
  const std::string &access_var_name,
  size_t size
) const
{
  SplitAccess split_access;
  if (is_a<ReadAction>(action))
  {
    // READ
    split_access.aux_variables = {
      {"read_value", "volatile char", std::to_string(size), ""},
      {"i", "volatile size_t"},
    };

    split_access.access_lines.push_back("for (i = 0; i < " + std::to_string(size) + "; i++)");
    split_access.access_lines.emplace_back("{");
    split_access.access_lines.push_back("  read_value[i] = " + access_var_name + "[i];");
    split_access.access_lines.emplace_back("}");
    split_access.access_lines.emplace_back("_use(read_value);" );
  }
  else
  {
    // WRITE
    split_access.aux_variables = {
      {"i", "volatile size_t", ""},
    };

    split_access.access_lines.push_back("for (i = 0; i < " + std::to_string(size) + "; i++)");
    split_access.access_lines.emplace_back("{");
    split_access.access_lines.push_back("  " + access_var_name + "[i] = 0xFF;");
    split_access.access_lines.emplace_back("}");

    split_access.access_lines.emplace_back("_use(" + access_var_name + ");" );
  }
  split_access.description = "constants";
  return split_access;
}

// generate from the given index to index + size
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

// generate using the given index up to index + distance
std::vector<std::string> DirectLocation::generate_using_runtime_index(
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

// generate in bulks using an auxiliary pointer
AccessLocation::SplitAccess DirectLocation::generate_bulk_split_using_index(
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
      {"tmp", "volatile char"},
      {"i", "volatile size_t"},
      {"reach_index", "volatile ssize_t", "", "0"},
    };

    split_access.result = "(" + from + " + reach_index)";
    split_access.access_lines.insert( split_access.access_lines.end(), {
      "while( GET_ADDR_BITS(&" + from + "[reach_index]) != GET_ADDR_BITS(" + to + ") )",
      "{",
      "  tmp = " + from + "[reach_index];",
      "  " + generate_counter_update("reach_index") + ";",
      "  _use(&tmp);",
      "}"
    });
    if (!distance.empty() && generate_preconditions_check_distance)
    {
      split_access.access_lines.insert(split_access.access_lines.begin(), "if ( !(" + generate_preconditions_check_distance(distance) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);");
    }
  }
  else
  {
    // WRITE
    // the caller must handle the allocation of reach_index, as it might be overwritten
    split_access.aux_variables = {
      {"reach_index", "ssize_t", "", "0"} // ssize_t reach_index = 0;
    };
    split_access.result = "(" + from + " + reach_index)";
    split_access.access_lines = {
      "while( GET_ADDR_BITS(&" + from + "[reach_index]) != GET_ADDR_BITS(" + to + ") )",
      "{",
      "  " + from + "[reach_index] = 0xFF;",
      "  " + generate_counter_update("reach_index") + ";",
      "  _use(&" + from + "[reach_index]);",
      "}",
    };
    if (!distance.empty() && generate_preconditions_check_distance)
    {
      split_access.access_lines.insert(split_access.access_lines.begin(), "if ( !(" + generate_preconditions_check_distance(distance) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);");
    }
    if (generate_preconditions_check_in_range)
    {
      split_access.access_lines.insert(split_access.access_lines.begin(), "if ( " + generate_preconditions_check_in_range("reach_index", from, to) + " ) _exit(PRECONDITIONS_FAILED_VALUE);");
    }
  }
  split_access.description = "index";
  return split_access;
}

// generate in bulks
AccessLocation::SplitAccess DirectLocation::generate_bulk_split_using_aux_ptr(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  std::string distance,
  std::function<std::string(const std::string&)>  generate_preconditions_check_distance,
  std::function<std::string(const std::string&, const std::string&, const std::string&)>  generate_preconditions_check_in_range,
  std::function<std::string(const std::string&)>  generate_counter_update
) const
{
  SplitAccess split_access;
  if (is_a<ReadAction>(action))
  {
    // READ
    split_access.aux_variables = {
      {"tmp", "volatile char"},
      {"aux_ptr", "volatile char *"},
      {"reach_index", "volatile ssize_t", "", "0"},
    };

    split_access.result = "aux_ptr";

    split_access.access_lines = { {
      "aux_ptr = &" + from + "[0];",
      "while( GET_ADDR_BITS(aux_ptr) != GET_ADDR_BITS(" + to + ") )",
      "{",
      "  tmp = *aux_ptr;",
      "  " + generate_counter_update("aux_ptr") + ";",
      "  _use(&tmp);",
      "}"
    } };
    if (!distance.empty() && generate_preconditions_check_distance)
    {
      split_access.access_lines.insert(split_access.access_lines.begin(), "if ( !(" + generate_preconditions_check_distance(distance) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);");
    }
  }
  else
  {
    // WRITE
    // the caller must handle the allocation of aux_ptr, as it might be overwritten
    split_access.aux_variables = {
      {"aux_ptr", "volatile char *"}
    };
    split_access.result = "aux_ptr";
    split_access.access_lines = {
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
      if (generate_preconditions_check_in_range) split_access.access_lines.insert(split_access.access_lines.begin(), "if ( " + generate_preconditions_check_in_range("aux_ptr", from, to) + " ) _exit(PRECONDITIONS_FAILED_VALUE);");
      if (generate_preconditions_check_distance) split_access.access_lines.insert(split_access.access_lines.begin(), "if ( !(" + generate_preconditions_check_distance(distance) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);");
    }
  }
  split_access.description = "auxiliary pointer";
  return split_access;
}

// generate using a load widening to uint32
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
      "_use(" + from + ");"
    };
    if (!distance.empty() && generate_preconditions_check_distance)
    {
      lines.insert(lines.begin(), "if ( !(" + distance + " < (" + std::to_string(size) + " + 3) ) ) _exit(PRECONDITIONS_FAILED_VALUE);");
      lines.insert(lines.begin(), "if ( !(" + generate_preconditions_check_distance(distance) + ") ) _exit(PRECONDITIONS_FAILED_VALUE);");
    }
  }
  return lines;
}

// generate after casting to uint8
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