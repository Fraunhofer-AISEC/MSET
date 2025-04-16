/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "underflow.h"

#include "misc.h"
#include "generator/primitives/access_types/direct_location.h"

bool Underflow::accepts(std::shared_ptr<AccessLocation> access_location) const
{
  return is_a<DirectLocation>(access_location);
}

bool Underflow::accepts_static_distance(ssize_t distance) const
{
  return distance <= 0;
}

std::string Underflow::generate_counter_update(const std::string &cnt) const
{
  return "--" + cnt;
}

std::string Underflow::generate_preconditions_check_distance( const std::string &distance ) const
{
  return distance + " <= 0";
}

std::string Underflow::generate_preconditions_check_in_range( const std::string &x, const std::string &from, const std::string &to ) const
{
  return "GET_ADDR_BITS(&" + x + ") < GET_ADDR_BITS(" + from + ") && GET_ADDR_BITS(&" + x + ") > GET_ADDR_BITS(" + to + ")";
}