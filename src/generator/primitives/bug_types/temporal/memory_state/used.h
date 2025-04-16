/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include "memory_state.h"


class UsedMemory: public MemoryState
{
public:
  UsedMemory();

  bool accepts(std::shared_ptr<Region> region) override;
};
