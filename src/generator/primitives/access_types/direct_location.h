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

  // simple generate
  std::vector<std::string> generate(
    std::shared_ptr<AccessAction> action,
    const std::string &access_var_name,
    size_t size
  ) const override;

  // simple split, using const size and content variables
  SplitAccess generate_split_aux_vars(
    std::shared_ptr<AccessAction> action,
    const std::string &access_var_name,
    size_t size
    ) const override;

  // simple split, using auxiliary size and content variables
  SplitAccess generate_split_const_vars(
    std::shared_ptr<AccessAction> action,
    const std::string &access_var_name,
    size_t size
  ) const override;

  // generate from the given index to index + size
  std::vector<std::string> generate_at_index(
    std::shared_ptr<AccessAction> action,
    const std::string &access_var_name,
    std::string index,
    size_t size,
    std::function<std::string(const std::string&)>  generate_preconditions_check_distance
  ) const override;

  // generate using the given index up to index + distance
  std::vector<std::string> generate_using_runtime_index(
    std::shared_ptr<AccessAction> action,
    const std::string &access_var_name,
    std::string index,
    std::string distance,
    std::function<std::string(const std::string&)> generate_preconditions_check_distance,
    std::function<std::string(const std::string&, const std::string&, const std::string&)> generate_preconditions_check_in_range
  ) const override;

  // generate in bulks
  SplitAccess generate_bulk_split_using_index(
    std::shared_ptr<AccessAction> action,
    std::string from,
    std::string to,
    std::string distance,
    std::function<std::string(const std::string&)>  generate_preconditions_check_distance,
    std::function<std::string(const std::string&, const std::string&, const std::string&)>  generate_preconditions_check_in_range,
    std::function<std::string(const std::string&)>  generate_counter_update
    ) const override;

  // generate in bulks using an auxiliary pointer
  SplitAccess generate_bulk_split_using_aux_ptr(
    std::shared_ptr<AccessAction> action,
    std::string from,
    std::string to,
    std::string distance,
    std::function<std::string(const std::string&)>  generate_preconditions_check_distance,
    std::function<std::string(const std::string&, const std::string&, const std::string&)>  generate_preconditions_check_in_range,
    std::function<std::string(const std::string&)>  generate_counter_update
  ) const override;

  // generate using a load widening to uint32
  std::vector<std::string> generate_uint32(
    std::shared_ptr<AccessAction> action,
    std::string from,
    std::string to,
    std::string distance,
    size_t size,
    std::function<std::string(const std::string&)>  generate_preconditions_check_distance
    ) const override;

  // generate after casting to uint8
  std::vector<std::string> generate_uint8(
    std::shared_ptr<AccessAction> action,
    std::string from,
    std::string to,
    std::string distance,
    size_t size,
    std::function<std::string(const std::string&)>  generate_preconditions_check_distance
  ) const override;
};
