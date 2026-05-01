/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once

#include "region.h"

class StackRegion: public Region
{
public:
  StackRegion();

  std::shared_ptr<RegionCodeCanvas> generate(std::shared_ptr<CodeCanvas> canvas, std::string name, size_t size, bool initialize) const override;
  std::shared_ptr<RegionCodeCanvas> generate(CodeCanvas::code_pos_t where, std::shared_ptr<CodeCanvas> canvas, std::string name, size_t size, bool initialize) const override;
  std::shared_ptr<RegionCodeCanvas> generate(
    std::shared_ptr<CodeCanvas> canvas,
    std::string name,
    std::string name_field_1, size_t size_field_1,
    std::string name_field_2, size_t size_field_2,
    bool initialize,
    size_t gap = 0
  ) const override;

  std::shared_ptr<RegionCodeCanvas> generate_array(std::shared_ptr<CodeCanvas> canvas, std::string name, size_t size, size_t array_size, bool initialize) const;
  std::shared_ptr<RegionCodeCanvas> generate_in_other_f(std::shared_ptr<CodeCanvas> canvas, std::string name, size_t size, bool initialize) const;

private:
  CodeCanvas::code_pos_t _generate_init_loop(
    std::shared_ptr<RegionCodeCanvas> canvas,
    CodeCanvas::code_pos_t where,
    const std::string &name,
    size_t size,
    const std::string &value,
    const std::string &indent,
    bool needs_strided = false,
    const std::string &offset = "0"
  ) const;

  CodeCanvas::code_pos_t _generate_2d_init_loop(
    std::shared_ptr<RegionCodeCanvas> canvas,
    CodeCanvas::code_pos_t where,
    const std::string &name,
    size_t size,
    size_t array_size,
    const std::string &value,
    const std::string &indent
  ) const;
};
