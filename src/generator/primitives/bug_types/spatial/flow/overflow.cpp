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

std::vector<std::string> Overflow::generate_counter_update( const std::string &cnt ) const
{
  return {"%" + cnt + "_next = arith.addi %" + cnt + ", %c1 : index"};
}

std::vector<std::string> Overflow::generate_preconditions_check_distance( const std::string &distance ) const
{
  return {
    "%is_valid = arith.cmpi sge, %" + distance + ", %c0 : index",
    "scf.if %is_valid {",
    "  func.call @exit(%precond_fail) : (i32) -> ()",
    "  scf.yield",
    "}",
  };
}

std::vector<std::string> Overflow::generate_preconditions_check_in_range( const std::string &x, const std::string &from, const std::string &to ) const
{
  (void)x; (void)from; (void)to;
  return {"%in _check_in_range = arith.constant 0 : index"};
}