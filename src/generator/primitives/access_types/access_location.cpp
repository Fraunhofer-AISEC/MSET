/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "access_location.h"

#include <utility>

AccessLocation::AccessLocation(std::string name):
  Property(std::move(name))
{
}

std::vector<AccessLocation::SplitAccess> AccessLocation::generate_split(
  std::shared_ptr<AccessAction> action,
  const std::string &access_var_name,
  size_t size
) const
{
  return { generate_split_aux_vars(action, access_var_name, size), generate_split_const_vars(action, access_var_name, size) };
}
