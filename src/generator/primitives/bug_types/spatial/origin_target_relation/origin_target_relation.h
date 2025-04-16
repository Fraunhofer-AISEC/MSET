/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include "generator/property.h"
#include "generator/primitives/regions/region.h"


class OriginTargetCodeCanvas: public CodeCanvas
{
public:
  OriginTargetCodeCanvas(
    const std::shared_ptr<RegionCodeCanvas>& code_canvas,
    size_t target_var_size,
    size_t origin_var_size,
    const std::string &target_var_name,
    const std::string &origin_var_name,
    const std::string &distance,
    const std::string &distance_negated,
    bool target_allocated = true,
    bool forces_underflow = false
    ):
      CodeCanvas(*code_canvas),
      target_var_size(target_var_size),
      origin_var_size(origin_var_size),
      target_var_name(target_var_name),
      origin_var_name(origin_var_name),
      distance(distance),
      distance_negated(distance_negated),
      target_allocated(target_allocated),
      forces_underflow(forces_underflow)
  {
  }

  OriginTargetCodeCanvas(const OriginTargetCodeCanvas &other) = default;

  OriginTargetCodeCanvas(OriginTargetCodeCanvas &&other) noexcept:
    CodeCanvas(std::move(other)),
    lifetime_pos(other.lifetime_pos),
    target_var_size(other.target_var_size),
    origin_var_size(other.origin_var_size),
    target_var_name(std::move(other.target_var_name)),
    origin_var_name(std::move(other.origin_var_name)),
    distance(std::move(other.distance)),
    distance_negated(std::move(other.distance_negated)),
    target_allocated(other.target_allocated),
    forces_underflow(other.forces_underflow)
  {
  }

  OriginTargetCodeCanvas & operator=(const OriginTargetCodeCanvas &other)
  {
    if (this == &other)
      return *this;
    CodeCanvas::operator=(other);
    lifetime_pos = other.lifetime_pos;
    target_var_size = other.target_var_size;
    origin_var_size = other.origin_var_size;
    target_var_name = other.target_var_name;
    origin_var_name = other.origin_var_name;
    distance = other.distance;
    distance_negated = other.distance_negated;
    target_allocated = other.target_allocated;
    forces_underflow = other.forces_underflow;
    return *this;
  }

  OriginTargetCodeCanvas & operator=(OriginTargetCodeCanvas &&other) noexcept
  {
    if (this == &other)
      return *this;
    CodeCanvas::operator=(std::move(other));
    lifetime_pos = other.lifetime_pos;
    target_var_size = other.target_var_size;
    origin_var_size = other.origin_var_size;
    target_var_name = other.target_var_name;
    origin_var_name = other.origin_var_name;
    distance = other.distance;
    distance_negated = other.distance_negated;
    target_allocated = other.target_allocated;
    forces_underflow = other.forces_underflow;
    return *this;
  }

  code_pos_t add_during_lifetime(const std::vector<std::string> &lines);
  code_pos_t add_during_lifetime(const std::string &line);

  void set_lifetime_pos(code_pos_t lifetime_pos) { this->lifetime_pos = lifetime_pos; }

  code_pos_t get_lifetime_pos() const
  {
    return lifetime_pos;
  }

  size_t get_target_size() const { return target_var_size; }
  size_t get_origin_size() const { return origin_var_size; }
  std::string get_target_name() const { return target_var_name; }
  std::string get_origin_name() const { return origin_var_name; }
  std::string get_distance() const {return distance; }
  std::string get_distance_negated() const {return distance_negated; }
  bool get_forces_underflow() const { return forces_underflow; }
  bool is_target_allocated() const { return target_allocated; }


private:
  void _update_indexes(code_pos_t from, size_t amount) override;

  code_pos_t lifetime_pos = -1 ;

  size_t target_var_size;
  size_t origin_var_size;
  std::string target_var_name;
  std::string origin_var_name;
  std::string distance;
  std::string distance_negated;

  bool target_allocated;
  bool forces_underflow;
};

class OriginTargetRelation: public Property
{
public:
  explicit OriginTargetRelation(const std::string &name):
    Property(name)
  {
  }
  virtual ~OriginTargetRelation() = default;
  virtual bool accepts(std::shared_ptr<Region> origin, std::shared_ptr<Region> target) const = 0;
  virtual std::vector< std::shared_ptr<OriginTargetCodeCanvas> > generate(
    CodeCanvas &canvas,
    std::shared_ptr<Region> origin,
    size_t origin_size,
    std::shared_ptr<Region> target,
    size_t target_size
  ) const = 0;
};
