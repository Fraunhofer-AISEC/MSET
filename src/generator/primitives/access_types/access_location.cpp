/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "access_location.h"

#include <iterator>
#include <utility>

AccessLocation::AccessLocation(std::string name):
  Property(std::move(name))
{
}

std::vector<AccessLocation::SplitAccess> AccessLocation::generate_split_all(
  std::shared_ptr<AccessAction> action,
  const std::string &access_var_name,
  size_t size,
  std::function<std::vector<std::string>(const std::string&)> generate_counter_update,
  const std::string &distance,
  bool needs_strided,
  const std::string &offset
) const
{
  return { generate_split_aux_vars(action, access_var_name, size,0, "", generate_counter_update,distance, needs_strided, offset), generate_split_const_vars(action, access_var_name, size, 0, "", needs_strided, offset) };
}

std::vector<AccessLocation::SplitAccess> AccessLocation::generate_bulk_split_all(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  std::string distance,
  std::function<std::vector<std::string>(const std::string&)>  generate_preconditions_check_distance,
  std::function<std::vector<std::string>(const std::string&, const std::string&, const std::string&)>  generate_preconditions_check_in_range,
  std::function<std::vector<std::string>(const std::string&)>  generate_counter_update,
  bool needs_strided,
  const std::string &offset
) const
{
  return {
    generate_bulk_split_using_index(action, from, to, distance, generate_preconditions_check_distance, generate_preconditions_check_in_range, generate_counter_update, needs_strided, offset),
    generate_bulk_split_using_aux_ptr(action, from, to, distance, generate_preconditions_check_distance, generate_preconditions_check_in_range, generate_counter_update, needs_strided, offset)
  };
}