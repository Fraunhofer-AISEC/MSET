/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include <memory>
#include <string>
#include <vector>
#include <functional>

#include "generator/property.h"
#include "generator/primitives/access_types/access_action.h"

class AccessLocation: public Property
{
public:
  struct SplitAccessLines
  {
    std::string aux_variable;
    std::string aux_variable_type;
    bool aux_variable_needs_allocation = false;
    std::string result;
    std::vector<std::string> access_lines;

    std::vector<std::string> to_lines() const
    {
      std::vector<std::string> lines = {aux_variable};
      lines.insert(lines.end(), access_lines.begin(), access_lines.end());
      return lines;
    }
  };
  virtual ~AccessLocation() = default;

  explicit AccessLocation(std::string name);

  // size known at compile-time
  virtual std::vector<std::string> generate(std::shared_ptr<AccessAction> action, const std::string &access_var_name, size_t size) const = 0;
  virtual SplitAccessLines generate_split(std::shared_ptr<AccessAction> action, const std::string &access_var_name, size_t size) const = 0;
  virtual SplitAccessLines generate_split_known_size(std::shared_ptr<AccessAction> action, const std::string &access_var_name, size_t size) const = 0;
  virtual std::vector<std::string> generate_at_index(
    std::shared_ptr<AccessAction> action,
    const std::string &access_var_name,
    std::string index,
    size_t size,
    std::function<std::string(const std::string&)>  generate_preconditions_check_distance
  ) const = 0;
  virtual std::vector<std::string> generate_with_runtime_index(
    std::shared_ptr<AccessAction> action,
    const std::string &access_var_name,
    std::string index,
    std::string distance,
    std::function<std::string(const std::string&)> generate_preconditions_check_distance,
    std::function<std::string(const std::string&, const std::string&, const std::string&)> generate_preconditions_check_in_range
) const = 0;
  // size unknown at compile-time
  virtual SplitAccessLines generate_bulk_split(
    std::shared_ptr<AccessAction> action,
    std::string from,
    std::string to,
    std::string distance,
    std::function<std::string(const std::string&)>  generate_preconditions_check_distance,
    std::function<std::string(const std::string&, const std::string&, const std::string&)>  generate_preconditions_check_in_range,
    std::function<std::string(const std::string&)>  generate_counter_update
    ) const = 0;
  virtual SplitAccessLines generate_bulk_split(
    std::shared_ptr<AccessAction> action,
    std::string from,
    std::string to,
    ssize_t distance,
    std::function<std::string(const std::string&, const std::string&, const std::string&)>  generate_preconditions_check_in_range,
    std::function<std::string(const std::string&)>  generate_counter_update
  ) const = 0;

  virtual std::vector<std::string> generate_uint32(
    std::shared_ptr<AccessAction> action,
    std::string from,
    std::string to,
    std::string distance,
    size_t size,
    std::function<std::string(const std::string&)>  generate_preconditions_check_distance
  ) const = 0;

  virtual std::vector<std::string> generate_uint8(
    std::shared_ptr<AccessAction> action,
    std::string from,
    std::string to,
    std::string distance,
    size_t size,
    std::function<std::string(const std::string&)>  generate_preconditions_check_distance
  ) const = 0;
};
