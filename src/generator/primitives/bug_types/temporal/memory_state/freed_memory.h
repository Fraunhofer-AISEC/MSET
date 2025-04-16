/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include "memory_state.h"

class FreedMemory: public MemoryState
{
public:
  FreedMemory();

  bool accepts(std::shared_ptr<Region> region) override;
};
