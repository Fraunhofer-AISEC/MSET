/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "overflow.h"

bool Overflow::accepts(std::shared_ptr<AccessLocation> access_location) const
{
  return true;
}

bool Overflow::accepts_static_distance(ssize_t distance) const
{
  return distance >= 0;
}

std::string Overflow::generate_counter_update( const std::string &cnt ) const
{
  return "++" + cnt;
}

std::string Overflow::generate_preconditions_check_distance( const std::string &distance ) const
{
  return distance + " >= 0";
}

std::string Overflow::generate_preconditions_check_in_range( const std::string &x, const std::string &from, const std::string &to ) const
{
  return "GET_ADDR_BITS(&" + x + ") < GET_ADDR_BITS(" + to + ") && GET_ADDR_BITS(&" + x + ") > GET_ADDR_BITS(" + from + ")";
}