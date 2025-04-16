/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "memory_state.h"

#include <cassert>

MemoryState::MemoryState(std::string name, std::string var_name):
  Property(name),
  var_name(var_name)
{
}
