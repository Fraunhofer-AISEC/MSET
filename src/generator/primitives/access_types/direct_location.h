/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include "generator/primitives/access_types/access_location.h"


class DirectLocation: public AccessLocation
{
public:

  DirectLocation():
    AccessLocation("direct")
  {
  }

  std::vector<std::string> generate(
    std::shared_ptr<AccessAction> action,
    const std::string &access_var_name,
    size_t size
  ) const override;
  std::vector<std::string> generate_at_index(
    std::shared_ptr<AccessAction> action,
    const std::string &access_var_name,
    std::string index,
    size_t size,
    std::function<std::string(const std::string&)>  generate_preconditions_check_distance
  ) const override;
  std::vector<std::string> generate_with_runtime_index(
    std::shared_ptr<AccessAction> action,
    const std::string &access_var_name,
    std::string index,
    std::string distance,
    std::function<std::string(const std::string&)> generate_preconditions_check_distance,
    std::function<std::string(const std::string&, const std::string&, const std::string&)> generate_preconditions_check_in_range
) const override;
  SplitAccessLines generate_split(
    std::shared_ptr<AccessAction> action,
    const std::string &access_var_name,
    size_t size
    ) const override;
  SplitAccessLines generate_split_known_size(
    std::shared_ptr<AccessAction> action,
    const std::string &access_var_name,
    size_t size
  ) const override;
  SplitAccessLines generate_bulk_split(
    std::shared_ptr<AccessAction> action,
    std::string from,
    std::string to,
    std::string distance,
    std::function<std::string(const std::string&)>  generate_preconditions_check_distance,
    std::function<std::string(const std::string&, const std::string&, const std::string&)>  generate_preconditions_check_in_range,
    std::function<std::string(const std::string&)>  generate_counter_update
    ) const override;
  SplitAccessLines generate_bulk_split(
    std::shared_ptr<AccessAction> action,
    std::string from,
    std::string to,
    ssize_t distance,
    std::function<std::string(const std::string&, const std::string&, const std::string&)>  generate_preconditions_check_in_range,
    std::function<std::string(const std::string&)>  generate_counter_update
  ) const override;

  std::vector<std::string> generate_uint32(
    std::shared_ptr<AccessAction> action,
    std::string from,
    std::string to,
    std::string distance,
    size_t size,
    std::function<std::string(const std::string&)>  generate_preconditions_check_distance
    ) const override;
  std::vector<std::string> generate_uint8(
    std::shared_ptr<AccessAction> action,
    std::string from,
    std::string to,
    std::string distance,
    size_t size,
    std::function<std::string(const std::string&)>  generate_preconditions_check_distance
  ) const override;
};
