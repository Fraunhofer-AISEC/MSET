/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include "generator/property.h"
#include "generator/primitives/access_types/access_location.h"
#include "generator/primitives/access_types/access_location.h"

class AccessLocation;

class Flow: public Property
{
public:
  explicit Flow(const std::string &name):
    Property(name)
  {
  }
  virtual ~Flow() = default;
  virtual bool accepts(std::shared_ptr<AccessLocation> access_location) const = 0;
  virtual bool accepts_static_distance(ssize_t distance) const = 0;
  virtual std::string generate_counter_update( const std::string &cnt ) const = 0;
  virtual std::string generate_preconditions_check_distance( const std::string &distance ) const = 0;
  virtual std::string generate_preconditions_check_in_range( const std::string &x, const std::string &from, const std::string &to ) const = 0;

};
