/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "generator/primitives/bug_types/temporal/memory_state/freed_memory.h"

#include "misc.h"
#include "generator/primitives/regions/global_region.h"

FreedMemory::FreedMemory():
  MemoryState("freed_memory", "target")
{
}

bool FreedMemory::accepts(std::shared_ptr<Region> region)
{
  return !is_a<GlobalRegion>(region);
}
