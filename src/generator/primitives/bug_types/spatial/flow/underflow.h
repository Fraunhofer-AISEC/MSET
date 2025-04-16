/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include "generator/primitives/bug_types/spatial/flow/flow.h"


class Underflow: public Flow
{
public:
  Underflow():
    Flow("underflow")
  {
  }

  bool accepts(std::shared_ptr<AccessLocation> access_location) const override;
  bool accepts_static_distance(ssize_t distance) const override;

  std::string generate_counter_update(const std::string &cnt) const override;
  std::string generate_preconditions_check_distance( const std::string &distance ) const override;
  std::string generate_preconditions_check_in_range( const std::string &x, const std::string &from, const std::string &to ) const override;
};
