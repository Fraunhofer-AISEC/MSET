/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "used.h"

#include <iostream>

UsedMemory::UsedMemory():
  MemoryState("used_memory", "target")
{
}

bool UsedMemory::accepts(std::shared_ptr<Region> region)
{
  return true;
}
