/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include "generator/primitives/access_types/access_action.h"


class WriteAction: public AccessAction
{
public:
  WriteAction(): AccessAction("write") {}
};
