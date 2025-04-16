/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once

#include <memory>
#include <vector>
#include <string>

#include "generator/code_canvas.h"
#include "generator/property.h"

class RegionCodeCanvas: public CodeCanvas
{
public:
  // RegionCodeCanvas();

  RegionCodeCanvas(const CodeCanvas & code_canvas, std::string var_size);
  RegionCodeCanvas(const CodeCanvas & code_canvas, size_t static_var_size);

  RegionCodeCanvas(const RegionCodeCanvas &other) = default;

  RegionCodeCanvas(RegionCodeCanvas &&other) noexcept:
    CodeCanvas(std::move(other)),
    allocation_pos(other.allocation_pos),
    deallocation_pos(other.deallocation_pos),
    lifetime_pos(other.lifetime_pos),
    var_size(std::move(other.var_size)),
    static_var_size(other.static_var_size)
  {
  }

  RegionCodeCanvas & operator=(const RegionCodeCanvas &other)
  {
    if (this == &other)
      return *this;
    CodeCanvas::operator=(other);
    allocation_pos = other.allocation_pos;
    deallocation_pos = other.deallocation_pos;
    lifetime_pos = other.lifetime_pos;
    var_size = other.var_size;
    static_var_size = other.static_var_size;
    return *this;
  }

  RegionCodeCanvas & operator=(RegionCodeCanvas &&other) noexcept
  {
    if (this == &other)
      return *this;
    CodeCanvas::operator=(std::move(other));
    allocation_pos = other.allocation_pos;
    deallocation_pos = other.deallocation_pos;
    lifetime_pos = other.lifetime_pos;
    var_size = other.var_size;
    static_var_size = other.static_var_size;
    return *this;
  }

  CodeCanvas::code_pos_t add_during_lifetime(const std::vector<std::string> &lines);
  CodeCanvas::code_pos_t add_during_lifetime(const std::string &line);

  void set_allocation_pos(CodeCanvas::code_pos_t allocation_pos) { this->allocation_pos = allocation_pos; }
  void set_deallocation_pos(CodeCanvas::code_pos_t deallocation_pos) { this->deallocation_pos = deallocation_pos; }
  void set_lifetime_pos(CodeCanvas::code_pos_t lifetime_pos) { this->lifetime_pos = lifetime_pos; }

  CodeCanvas::code_pos_t get_allocation_pos() const
  {
    return allocation_pos;
  }

  CodeCanvas::code_pos_t get_deallocation_pos() const
  {
    return deallocation_pos;
  }

  CodeCanvas::code_pos_t get_lifetime_pos() const
  {
    return lifetime_pos;
  }

  std::string get_var_size() const
  {
    if (!var_size.empty()) return var_size;
    return std::to_string(static_var_size);
  }
  size_t get_static_var_size() const { return static_var_size; }

private:
  void _update_indexes(CodeCanvas::code_pos_t from, size_t amount) override;

  CodeCanvas::code_pos_t allocation_pos;
  CodeCanvas::code_pos_t deallocation_pos;
  CodeCanvas::code_pos_t lifetime_pos;

  std::string var_size;
  size_t static_var_size;
};

class Region: public Property
{
public:
  explicit Region(std::string name);
  virtual ~Region() = default;

  virtual std::shared_ptr<RegionCodeCanvas> generate(std::shared_ptr<CodeCanvas> canvas, std::string name, size_t size, bool initialize) const = 0;
  virtual std::shared_ptr<RegionCodeCanvas> generate(CodeCanvas::code_pos_t where, std::shared_ptr<CodeCanvas> canvas, std::string name, size_t size, bool initialize) const = 0;
  virtual std::shared_ptr<RegionCodeCanvas> generate(
    std::shared_ptr<CodeCanvas> canvas,
    std::string name,
    std::string name_field_1, size_t size_field_1,
    std::string name_field_2, size_t size_field_2,
    bool initialize
  ) const = 0;
};
