/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "stdlib_location.h"

#include <cassert>

#include "misc.h"
#include "generator/primitives/access_types/read_action.h"

std::vector<std::string> StdlibLocation::generate(
  std::shared_ptr<AccessAction> action, const std::string &access_var_name, size_t size, size_t array_size, const std::string &index_var) const
{
  return generate_split_const_vars(action, access_var_name, size, array_size, index_var).to_lines();
}

AccessLocation::SplitAccess StdlibLocation::generate_split_aux_vars(
  std::shared_ptr<AccessAction> action, const std::string &access_var_name, size_t size, size_t array_size, const std::string &index_var) const
{
  SplitAccess split_access;
  std::string size_str = std::to_string(size);
  if (is_a<ReadAction>(action))
  {
    // READ: memref.copy
    if (array_size > 0) {
      std::string array_size_str = std::to_string(array_size);
      split_access.aux_variables = {
        {"read_value", "memref<1x" + size_str + "xi8>", "", "memref.alloca() : memref<1x" + size_str + "xi8>"},
      };
      split_access.access_lines = {
        "%subview_tmp = memref.subview %" + access_var_name + "[%" + index_var + ", 0][1, " + size_str + "][1, 1] : memref<" + array_size_str + "x" + size_str + "xi8> to memref<1x" + size_str + "xi8>",
        "memref.copy %subview_tmp, %read_value : memref<1x" + size_str + "xi8> to memref<1x" + size_str + "xi8>"
      };
    } else {
      split_access.aux_variables = {
        {"read_value", "memref<" + size_str + "xi8>", "", "memref.alloca() : memref<" + size_str + "xi8>"},
      };
      split_access.access_lines.push_back( "memref.copy %" + access_var_name + ", %read_value : memref<" + size_str + "xi8> to memref<" + size_str + "xi8>" );
    }
    split_access.result = access_var_name;
  }
  else
  {
    // WRITE: scf.for + store
    split_access.aux_variables = {
      {"c0", "index", "", "0"},
      {"c1", "index", "", "1"},
      {"c" + size_str, "index", "", size_str},
      {"c0xFF", "i8", "", "255"},
    };
    if (array_size > 0) {
      std::string array_size_str = std::to_string(array_size);
      split_access.access_lines.emplace_back("scf.for %i = %c0 to %c" + size_str + " step %c1 {");
        split_access.access_lines.emplace_back("  memref.store %c0xFF, %" + access_var_name + "[%" + index_var + ", %i] : memref<" + array_size_str + "x" + size_str + "xi8>");
      split_access.access_lines.emplace_back("}");
    } else {
      split_access.access_lines.emplace_back("scf.for %i = %c0 to %c" + size_str + " step %c1 {");
        split_access.access_lines.emplace_back("  memref.store %c0xFF, %" + access_var_name + "[%i] : memref<" + size_str + "xi8>");
      split_access.access_lines.emplace_back("}");
    }
    split_access.result = access_var_name;
  }
  split_access.description = "auxiliary variables";
  return split_access;
}

AccessLocation::SplitAccess StdlibLocation::generate_split_const_vars(
  std::shared_ptr<AccessAction> action, const std::string &access_var_name, size_t size, size_t array_size, const std::string &index_var) const
{
  SplitAccess split_access;
  std::string size_str = std::to_string(size);
  if (is_a<ReadAction>(action))
  {
    // READ: memref.copy
    if (array_size > 0) {
      std::string array_size_str = std::to_string(array_size);
      split_access.aux_variables = {
        {"read_value", "memref<1x" + size_str + "xi8>", "", "memref.alloca() : memref<1x" + size_str + "xi8>"},
      };
      split_access.access_lines = {
        "%subview_tmp = memref.subview %" + access_var_name + "[%" + index_var + ", 0][1, " + size_str + "][1, 1] : memref<" + array_size_str + "x" + size_str + "xi8> to memref<1x" + size_str + "xi8>",
        "memref.copy %subview_tmp, %read_value : memref<1x" + size_str + "xi8> to memref<1x" + size_str + "xi8>"
      };
    } else {
      split_access.aux_variables = {
        {"read_value", "memref<" + size_str + "xi8>", "", "memref.alloca() : memref<" + size_str + "xi8>"},
      };
      split_access.access_lines.push_back( "memref.copy %" + access_var_name + ", %read_value : memref<" + size_str + "xi8> to memref<" + size_str + "xi8>" );
    }
    split_access.result = access_var_name;
  }
  else
  {
    // WRITE: scf.for + store
    split_access.aux_variables = {
      {"c0", "index", "", "0"},
      {"c1", "index", "", "1"},
      {"c" + size_str, "index", "", size_str},
      {"c0xFF", "i8", "", "255"},
    };
    if (array_size > 0) {
      std::string array_size_str = std::to_string(array_size);
      split_access.access_lines.emplace_back("scf.for %i = %c0 to %c" + size_str + " step %c1 {");
        split_access.access_lines.emplace_back("  memref.store %c0xFF, %" + access_var_name + "[%" + index_var + ", %i] : memref<" + array_size_str + "x" + size_str + "xi8>");
      split_access.access_lines.emplace_back("}");
    } else {
      split_access.access_lines.emplace_back("scf.for %i = %c0 to %c" + size_str + " step %c1 {");
        split_access.access_lines.emplace_back("  memref.store %c0xFF, %" + access_var_name + "[%i] : memref<" + size_str + "xi8>");
      split_access.access_lines.emplace_back("}");
    }
    split_access.result = access_var_name;
  }
  split_access.description = "constants";
  return split_access;
}

AccessLocation::SplitAccess StdlibLocation::generate_bulk_split_using_index(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  std::string distance,
  std::function<std::vector<std::string>(const std::string&)> generate_preconditions_check_distance,
  std::function<std::vector<std::string>(const std::string&, const std::string&, const std::string&)> generate_preconditions_check_in_range,
  std::function<std::vector<std::string>(const std::string&)> generate_counter_update
) const
{
  SplitAccess split_access;
  if ( distance.empty() )
  {
    distance = "distance"; // placeholder; caller provides actual distance SSA
  }
  if (is_a<ReadAction>(action))
  {
    // READ: dynamic chunk copy using memref.copy + subview
    split_access.aux_variables = {
      {"c0", "index", "", "0"},
      {"c1", "index", "", "1"},
      {"c1024", "index", "", "1024"},
      {"read_value", "memref<1024xi8>", "", "memref.alloca() : memref<1024xi8>"},
    };

    split_access.access_lines = {
      "scf.for %i = %c0 to %" + distance + " step %c1024 {",
      "  %remaining = arith.subi %" + distance + ", %i : index",
      "  %is_full = arith.cmpi sgt, %remaining, %c1024 : index",
      "  %step = arith.select %is_full, %c1024, %remaining : index",
      "  %src_slice = memref.subview %" + from + "[%i][%step][1] : memref<8xi8> to memref<?xi8>",
      "  %dst_slice = memref.subview %read_value[%c0][%step][1] : memref<1024xi8> to memref<?xi8>",
      "  memref.copy %src_slice, %dst_slice : memref<?xi8> to memref<?xi8>",
      "}",
    };
    split_access.result = from;
  }
  else
  {
    // WRITE: scf.for + store per byte
    split_access.aux_variables = {
      {"c0", "index", "", "0"},
      {"c1", "index", "", "1"},
      {"c0xFF", "i8", "", "255"},
    };
    split_access.access_lines = {
      "scf.for %i = %c0 to %" + distance + " step %c1 {",
      "  memref.store %c0xFF, %" + from + "[%i] : memref<8xi8>",
      "}",
    };
    split_access.result = from;
  }
  split_access.description = "index";
  return split_access;
}

AccessLocation::SplitAccess StdlibLocation::generate_bulk_split_using_aux_ptr(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  std::string distance,
  std::function<std::vector<std::string>(const std::string&)>  generate_preconditions_check_distance,
  std::function<std::vector<std::string>(const std::string&, const std::string&, const std::string&)> generate_preconditions_check_in_range,
  std::function<std::vector<std::string>(const std::string&)> generate_counter_update
) const
{
  SplitAccess split_access;

  if (is_a<ReadAction>(action))
  {
    // READ: dynamic chunk copy using memref.copy + subview
    split_access.aux_variables = {
      {"c0", "index", "", "0"},
      {"c1", "index", "", "1"},
      {"c1024", "index", "", "1024"},
      {"read_value", "memref<1024xi8>", "", "memref.alloca() : memref<1024xi8>"},
    };
    split_access.access_lines = {
      "scf.for %i = %c0 to %" + distance + " step %c1024 {",
      "  %remaining = arith.subi %" + distance + ", %i : index",
      "  %is_full = arith.cmpi sgt, %remaining, %c1024 : index",
      "  %step = arith.select %is_full, %c1024, %remaining : index",
      "  %src_slice = memref.subview %" + from + "[%i][%step][1] : memref<8xi8> to memref<?xi8>",
      "  %dst_slice = memref.subview %read_value[%c0][%step][1] : memref<1024xi8> to memref<?xi8>",
      "  memref.copy %src_slice, %dst_slice : memref<?xi8> to memref<?xi8>",
      "}",
    };
    split_access.result = from;
  }
  else
  {
    // WRITE: scf.for + store per byte
    split_access.aux_variables = {
      {"c0", "index", "", "0"},
      {"c1", "index", "", "1"},
      {"c0xFF", "i8", "", "255"},
    };
    split_access.access_lines = {
      "scf.for %i = %c0 to %" + distance + " step %c1 {",
      "  memref.store %c0xFF, %" + from + "[%i] : memref<8xi8>",
      "}",
    };
    split_access.result = from;
  }

  split_access.description = "auxiliary pointer";
  return split_access;
}


std::vector<std::string> StdlibLocation::generate_at_index(
  std::shared_ptr<AccessAction> action,
  const std::string &access_var_name,
  std::string index,
  size_t size,
  std::function<std::vector<std::string>(const std::string&)> generate_preconditions_check_distance
) const
{
  std::vector<std::string> lines;
  std::string size_str = std::to_string(size);
  if (is_a<ReadAction>(action))
  {
    // READ: memref.copy via subview
    lines = {
      "%read_value = memref.alloca() : memref<" + size_str + "xi8>",
      "%src_slice = memref.subview %" + access_var_name + "[" + index + "][" + size_str + "][1] : memref<" + size_str + "xi8> to memref<" + size_str + "xi8>",
      "memref.copy %src_slice, %read_value : memref<" + size_str + "xi8> to memref<" + size_str + "xi8>",
    };
  }
  else
  {
    // WRITE: scf.for + store
    lines = {
      "%c0 = arith.constant 0 : index",
      "%c1 = arith.constant 1 : index",
      "%c" + size_str + " = arith.constant " + size_str + " : index",
      "%c0xFF = arith.constant 255 : i8",
      "scf.for %j = %c0 to %c" + size_str + " step %c1 {",
      "  %idx = arith.addi %j, %" + index + " : index",
      "  memref.store %c0xFF, %" + access_var_name + "[%idx] : memref<" + size_str + "xi8>",
      "}",
    };
  }
  return lines;
}


std::vector<std::string> StdlibLocation::generate_using_runtime_index(
  std::shared_ptr<AccessAction> action,
  const std::string &access_var_name,
  std::string index,
  std::string distance,
  std::function<std::vector<std::string>(const std::string&)> generate_preconditions_check_distance,
  std::function<std::vector<std::string>(const std::string&, const std::string&, const std::string&)> generate_preconditions_check_in_range
) const
{
  assert(0 && "generate_using_runtime_index not used for StdlibLocation");
  return {};
}

std::vector<std::string> StdlibLocation::generate_uint32(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  std::string distance,
  size_t size,
  std::function<std::vector<std::string>(const std::string&)>  generate_preconditions_check_distance
) const
{
  assert(0 && "generate_uint32 not used for StdlibLocation");
  return {};
}

std::vector<std::string> StdlibLocation::generate_uint8(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  std::string distance,
  size_t size,
  std::function<std::vector<std::string>(const std::string&)>  generate_preconditions_check_distance
) const
{
  assert(0 && "generate_uint8 not used for StdlibLocation");
  return {};
}