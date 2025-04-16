/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "temporal_bug_type.h"

#include <utility>

TemporalBugType::TemporalBugType(std::string name):
  Property(std::move(name))
{
}
