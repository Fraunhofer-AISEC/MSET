/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "access_location.h"

#include <utility>

AccessLocation::AccessLocation(std::string name):
  Property(std::move(name))
{
}
