/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include <algorithm>
#include <string>


class Property
{
public:
  explicit Property(std::string name):
    name(name)
  {}

  std::string get_name() const { return name; }
  std::string get_printable_name() const
  {
    std::string printable_name = name;
    std::replace( printable_name.begin(), printable_name.end(), '_', '-' );
    return printable_name;
  }
private:
  std::string name;
};
