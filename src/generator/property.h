/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include <string>


class Property
{
public:
  explicit Property(std::string name):
    name(name)
  {}

  std::string get_name() const { return name; }
private:
  std::string name;
};
