/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <functional>

#include "generator/property.h"
#include "generator/primitives/access_types/access_action.h"

class AccessLocation: public Property
{
public:
  struct AuxiliaryVariable
  {
    std::string name;
    std::string type;
    std::string number_of_elements;
    std::string init_value;

    std::string to_string() const
    {
      std::string str = type + " " + name;
      if ( !number_of_elements.empty() )
      {
        str += "[" + number_of_elements + "]";
      }
      if ( !init_value.empty() )
      {
        str += " = " + init_value;
      }
      return str + ";";
    }

    AuxiliaryVariable(const AuxiliaryVariable &other) = default;

    AuxiliaryVariable(AuxiliaryVariable &&other) noexcept :
      name(std::move(other.name)),
      type(std::move(other.type)),
      number_of_elements(std::move(other.number_of_elements)),
      init_value(std::move(other.init_value))
    {
    }

    AuxiliaryVariable & operator=(const AuxiliaryVariable &other)
    {
      if (this == &other)
        return *this;
      name = other.name;
      type = other.type;
      number_of_elements = other.number_of_elements;
      init_value = other.init_value;
      return *this;
    }

    AuxiliaryVariable & operator=(AuxiliaryVariable &&other) noexcept
    {
      if (this == &other)
        return *this;
      name = std::move(other.name);
      type = std::move(other.type);
      number_of_elements = std::move(other.number_of_elements);
      init_value = std::move(other.init_value);
      return *this;
    }

    AuxiliaryVariable() = default;

    AuxiliaryVariable(
      std::string name,
      std::string type,
      std::string number_of_elements = "",
      std::string init_value = "") :
      name{std::move(name)},
      type{std::move(type)},
      number_of_elements{std::move(number_of_elements)},
      init_value{std::move(init_value)}
    {
    }

    static std::vector< std::vector<std::string> > to_string_vector( const std::vector<AuxiliaryVariable> & auxiliary_variables )
    {
      std::vector< std::vector<std::string> > result;
      result.reserve( auxiliary_variables.size() );
      std::transform(auxiliary_variables.begin(), auxiliary_variables.end(), std::back_inserter(result),
        [](const AuxiliaryVariable & auxiliary_variable) { return std::vector< std::string>{ auxiliary_variable.to_string() }; }
      );
      return result;
    }
  };
  struct SplitAccess
  {
    std::vector<AuxiliaryVariable> aux_variables;
    std::string result;
    std::vector<std::string> access_lines;

    std::vector<std::string> to_lines() const
    {
      std::vector<std::string> lines;
      std::transform(aux_variables.begin(), aux_variables.end(), std::back_inserter(lines),
        [](const AuxiliaryVariable& var)
        {
          return var.to_string() + ";";
        }
      );
      std::transform(aux_variables.begin(), aux_variables.end(), lines.begin(),
                   [](const AuxiliaryVariable& obj) { return obj.to_string(); });
      lines.insert(lines.end(), access_lines.begin(), access_lines.end());
      return lines;
    }

    SplitAccess() = default;
    SplitAccess(const SplitAccess &other) = default;

    SplitAccess(SplitAccess &&other) noexcept :
      aux_variables(std::move(other.aux_variables)),
      result(std::move(other.result)),
      access_lines(std::move(other.access_lines))
    {
    }

    SplitAccess & operator=(const SplitAccess &other)
    {
      if (this == &other)
        return *this;
      aux_variables = other.aux_variables;
      result = other.result;
      access_lines = other.access_lines;
      return *this;
    }

    SplitAccess & operator=(SplitAccess &&other) noexcept
    {
      if (this == &other)
        return *this;
      aux_variables = std::move(other.aux_variables);
      result = std::move(other.result);
      access_lines = std::move(other.access_lines);
      return *this;
    }
  };
  virtual ~AccessLocation() = default;

  explicit AccessLocation(std::string name);

  // size known at compile-time
  // simple generate
  virtual std::vector<std::string> generate(std::shared_ptr<AccessAction> action, const std::string &access_var_name, size_t size) const = 0;


  // simple split, all variants
  std::vector<SplitAccess> generate_split(std::shared_ptr<AccessAction> action, const std::string &access_var_name, size_t size) const;
  // simple split, using auxiliary size and content variables
  virtual SplitAccess generate_split_aux_vars(std::shared_ptr<AccessAction> action, const std::string &access_var_name, size_t size) const = 0;
  // simple split, using const size and content variables
  virtual SplitAccess generate_split_const_vars(std::shared_ptr<AccessAction> action, const std::string &access_var_name, size_t size) const = 0;

  // generate from the given index to index + size
  virtual std::vector<std::string> generate_at_index(
    std::shared_ptr<AccessAction> action,
    const std::string &access_var_name,
    std::string index,
    size_t size,
    std::function<std::string(const std::string&)>  generate_preconditions_check_distance
  ) const = 0;

  // generate using the given index up to index + distance
  virtual std::vector<std::string> generate_using_runtime_index(
    std::shared_ptr<AccessAction> action,
    const std::string &access_var_name,
    std::string index,
    std::string distance,
    std::function<std::string(const std::string&)> generate_preconditions_check_distance,
    std::function<std::string(const std::string&, const std::string&, const std::string&)> generate_preconditions_check_in_range
  ) const = 0;

  // generate in bulks
  virtual SplitAccess generate_bulk_split(
    std::shared_ptr<AccessAction> action,
    std::string from,
    std::string to,
    std::string distance,
    std::function<std::string(const std::string&)>  generate_preconditions_check_distance,
    std::function<std::string(const std::string&, const std::string&, const std::string&)>  generate_preconditions_check_in_range,
    std::function<std::string(const std::string&)>  generate_counter_update
    ) const = 0;

  // generate in bulks using an auxiliary pointer
  virtual SplitAccess generate_bulk_split_using_aux_ptr(
    std::shared_ptr<AccessAction> action,
    std::string from,
    std::string to,
    std::string distance,
    std::function<std::string(const std::string&)>  generate_preconditions_check_distance,
    std::function<std::string(const std::string&, const std::string&, const std::string&)>  generate_preconditions_check_in_range,
    std::function<std::string(const std::string&)>  generate_counter_update
  ) const = 0;

  // generate using a load widening to uint32
  virtual std::vector<std::string> generate_uint32(
    std::shared_ptr<AccessAction> action,
    std::string from,
    std::string to,
    std::string distance,
    size_t size,
    std::function<std::string(const std::string&)>  generate_preconditions_check_distance
  ) const = 0;

  // generate after casting to uint8
  virtual std::vector<std::string> generate_uint8(
    std::shared_ptr<AccessAction> action,
    std::string from,
    std::string to,
    std::string distance,
    size_t size,
    std::function<std::string(const std::string&)>  generate_preconditions_check_distance
  ) const = 0;
};
