/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include <string>

#include "generator/property.h"


class AccessAction: public Property
{
public:
  virtual ~AccessAction() = default;
  explicit AccessAction(std::string name): Property(name) {}
};
