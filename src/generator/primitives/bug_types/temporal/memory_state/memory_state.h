/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include <memory>
#include <string>

#include "generator/code_canvas.h"
#include "generator/property.h"
#include "generator/primitives/regions/region.h"

class MemoryState: public Property
{
public:
  MemoryState(std::string name, std::string var_name);
  virtual ~MemoryState() = default;

  virtual bool accepts(std::shared_ptr<Region> region) = 0;
private:
  std::string var_name;
};
