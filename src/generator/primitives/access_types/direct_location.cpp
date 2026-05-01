/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "direct_location.h"

#include <iostream>
#include <ostream>
#include <string>

#include "misc.h"
#include "generator/primitives/access_types/read_action.h"
#include "generator/primitives/bug_types/spatial/flow/flow.h"

static bool isUnderFlow( std::function<std::vector<std::string>(const std::string&)> generate_counter_update){
  bool is_underflow = false;
  std::vector<std::string> counter_update = generate_counter_update("reach_index");
  for (const auto& line : counter_update)
  {
    if (line.find("subi") != std::string::npos)
    {
      is_underflow = true;
      break;
    }
  }
  return is_underflow;
}
// simple generate
std::vector<std::string> DirectLocation::generate(std::shared_ptr<AccessAction> action, const std::string &access_var_name, size_t size, size_t array_size, const std::string &index_var, bool needs_strided, const std::string &offset) const
{
  return generate_split_const_vars(action, access_var_name, size, array_size, index_var, needs_strided, offset).to_lines();
}

// simple split, using auxiliary size and content variables
AccessLocation::SplitAccess DirectLocation::generate_split_aux_vars(
  std::shared_ptr<AccessAction> action,
  const std::string &access_var_name,
  size_t size,
  size_t array_size,
  const std::string &index_var,
  std::function<std::vector<std::string>(const std::string&)> generate_counter_update,
  const std::string &distance,
  bool needs_strided,
  const std::string &offset
) const
{
  SplitAccess split_access;
  std::string size_str = std::to_string(size);
  std::string stride_suffix = needs_strided ? ", strided<[1], offset: " + offset + ">>" : ">";
  if (is_a<ReadAction>(action))
  {
    // READ
    split_access.aux_variables = {
      {"c0", "index", "", "0"},
      {"c1", "index", "", "1"},
      {"c" + size_str, "index", "", size_str},
    };

    if (array_size > 0) {
      std::string array_size_str = std::to_string(array_size);
      split_access.access_lines.emplace_back("scf.for %i = %c0 to %c" + size_str + " step %c1 {");
      {
        std::string idx_expr = "%" + index_var + ", %i";
        if (!offset.empty() && offset != "0") {
          split_access.access_lines.emplace_back("  %__base = arith.constant " + offset + " : index");
          split_access.access_lines.emplace_back("  %__idx = arith.addi %" + index_var + ", %__base : index");
          idx_expr = "%__idx, %i";
        }
        split_access.access_lines.emplace_back("  %val = memref.load %" + access_var_name + "[" + idx_expr + "] : memref<" + array_size_str + "x" + size_str + "xi8" + stride_suffix);
      }
      split_access.access_lines.emplace_back("}");
    } else {
      split_access.access_lines.emplace_back("scf.for %i = %c0 to %c" + size_str + " step %c1 {");
      {
        std::string idx_expr = "%i";
        if (!offset.empty() && offset != "0") {
          split_access.access_lines.emplace_back("  %__base = arith.constant " + offset + " : index");
          split_access.access_lines.emplace_back("  %__idx = arith.addi %i, %__base : index");
          idx_expr = "%__idx";
        }
        split_access.access_lines.emplace_back("  %val = memref.load %" + access_var_name + "[" + idx_expr + "] : memref<" + size_str + "xi8" + stride_suffix);
      }
      split_access.access_lines.emplace_back("}");
    }
  }
  else
  {
    // WRITE
    split_access.aux_variables = {
      {"c0", "index", "", "0"},
      {"c1", "index", "", "1"},
      {"c" + size_str, "index", "", size_str},
      {"c0xFF", "i8", "", "255"},
    };
    if (array_size > 0) {
      std::string array_size_str = std::to_string(array_size);
      split_access.access_lines.emplace_back("scf.for %i = %c0 to %c" + size_str + " step %c1 {");
      {
        std::string idx_expr = "%" + index_var + ", %i";
        if (!offset.empty() && offset != "0") {
          split_access.access_lines.emplace_back("  %__base = arith.constant " + offset + " : index");
          split_access.access_lines.emplace_back("  %__idx = arith.addi %" + index_var + ", %__base : index");
          idx_expr = "%__idx, %i";
        }
        split_access.access_lines.emplace_back("  memref.store %c0xFF, %" + access_var_name + "[" + idx_expr + "] : memref<" + array_size_str + "x" + size_str + "xi8" + stride_suffix);
      }
      split_access.access_lines.emplace_back("}");
    } else {
      split_access.access_lines.emplace_back("scf.for %i = %c0 to %c" + size_str + " step %c1 {");
      {
        std::string idx_expr = "%i";
        if (!offset.empty() && offset != "0") {
          split_access.access_lines.emplace_back("  %__base = arith.constant " + offset + " : index");
          split_access.access_lines.emplace_back("  %__idx = arith.addi %i, %__base : index");
          idx_expr = "%__idx";
        }
        split_access.access_lines.emplace_back("  memref.store %c0xFF, %" + access_var_name + "[" + idx_expr + "] : memref<" + size_str + "xi8" + stride_suffix);
      }
      split_access.access_lines.emplace_back("}");
    }
  }
  split_access.description = "auxiliary variables";
  return split_access;
}

// simple split, using const size and content variables
AccessLocation::SplitAccess DirectLocation::generate_split_const_vars(
  std::shared_ptr<AccessAction> action,
  const std::string &access_var_name,
  size_t size,
  size_t array_size,
  const std::string &index_var,
  bool needs_strided,
  const std::string &offset
) const
{
  SplitAccess split_access;
  std::string size_str = std::to_string(size);
  std::string stride_suffix = needs_strided ? ", strided<[1], offset: " + offset + ">>" : ">";
  if (is_a<ReadAction>(action))
  {
    // READ
    split_access.aux_variables = {
      {"c0", "index", "", "0"},
      {"c1", "index", "", "1"},
      {"c" + size_str, "index", "", size_str},
    };

    if (array_size > 0) {
      std::string array_size_str = std::to_string(array_size);
      split_access.access_lines.emplace_back("scf.for %i = %c0 to %c" + size_str + " step %c1 {");
      {
        std::string idx_expr = "%" + index_var + ", %i";
        if (!offset.empty() && offset != "0") {
          split_access.access_lines.emplace_back("  %__base = arith.constant " + offset + " : index");
          split_access.access_lines.emplace_back("  %__idx = arith.addi %" + index_var + ", %__base : index");
          idx_expr = "%__idx, %i";
        }
        split_access.access_lines.emplace_back("  %val = memref.load %" + access_var_name + "[" + idx_expr + "] : memref<" + array_size_str + "x" + size_str + "xi8" + stride_suffix);
      }
      split_access.access_lines.emplace_back("}");
    } else {
      split_access.access_lines.emplace_back("scf.for %i = %c0 to %c" + size_str + " step %c1 {");
      {
        std::string idx_expr = "%i";
        if (!offset.empty() && offset != "0") {
          split_access.access_lines.emplace_back("  %__base = arith.constant " + offset + " : index");
          split_access.access_lines.emplace_back("  %__idx = arith.addi %i, %__base : index");
          idx_expr = "%__idx";
        }
        split_access.access_lines.emplace_back("  %val = memref.load %" + access_var_name + "[" + idx_expr + "] : memref<" + size_str + "xi8" + stride_suffix);
      }
      split_access.access_lines.emplace_back("}");
    }
  }
  else
  {
    // WRITE
    split_access.aux_variables = {
      {"c0", "index", "", "0"},
      {"c1", "index", "", "1"},
      {"c" + size_str, "index", "", size_str},
      {"c0xFF", "i8", "", "255"},
    };
    if (array_size > 0) {
      std::string array_size_str = std::to_string(array_size);
      split_access.access_lines.emplace_back("scf.for %i = %c0 to %c" + size_str + " step %c1 {");
      {
        std::string idx_expr = "%" + index_var + ", %i";
        if (!offset.empty() && offset != "0") {
          split_access.access_lines.emplace_back("  %__base = arith.constant " + offset + " : index");
          split_access.access_lines.emplace_back("  %__idx = arith.addi %" + index_var + ", %__base : index");
          idx_expr = "%__idx, %i";
        }
        split_access.access_lines.emplace_back("  memref.store %c0xFF, %" + access_var_name + "[" + idx_expr + "] : memref<" + array_size_str + "x" + size_str + "xi8" + stride_suffix);
      }
      split_access.access_lines.emplace_back("}");
    } else {
      split_access.access_lines.emplace_back("scf.for %i = %c0 to %c" + size_str + " step %c1 {");
      {
        std::string idx_expr = "%i";
        if (!offset.empty() && offset != "0") {
          split_access.access_lines.emplace_back("  %__base = arith.constant " + offset + " : index");
          split_access.access_lines.emplace_back("  %__idx = arith.addi %i, %__base : index");
          idx_expr = "%__idx";
        }
        split_access.access_lines.emplace_back("  memref.store %c0xFF, %" + access_var_name + "[" + idx_expr + "] : memref<" + size_str + "xi8" + stride_suffix);
      }
      split_access.access_lines.emplace_back("}");
    }
  }
  split_access.description = "constants";
  return split_access;
}

// generate from the given index to index + size
std::vector<std::string> DirectLocation::generate_at_index(
  std::shared_ptr<AccessAction> action,
  const std::string &access_var_name,
  std::string index,
  size_t size,
  std::function<std::vector<std::string>(const std::string&)> generate_preconditions_check_distance,
  bool needs_strided,
  const std::string &offset
) const
{
  std::vector<std::string> lines;
  std::string size_str = std::to_string(size);
  std::string stride_suffix = needs_strided ? ", strided<[1], offset: " + offset + ">>" : ">";
  if (is_a<ReadAction>(action))
  {
    // READ
    lines = {
      "%c0 = arith.constant 0 : index",
      "%c1 = arith.constant 1 : index",
      "%c" + size_str + " = arith.constant " + size_str + " : index",
      "scf.for %j = %c0 to %c" + size_str + " step %c1 {",
      "  %idx = arith.addi %j, %" + index + " : index",
      "  %val = memref.load %" + access_var_name + "[%idx] : memref<" + size_str + "xi8" + stride_suffix,
      "}",
    };

    if (generate_preconditions_check_distance)
    {
      // precondition checks are MLIR-style now; integrate if needed
    };
  }
  else
  {
    // WRITE
    lines = {
      "%c0 = arith.constant 0 : index",
      "%c1 = arith.constant 1 : index",
      "%c" + size_str + " = arith.constant " + size_str + " : index",
      "%c0xFF = arith.constant 255 : i8",
      "scf.for %j = %c0 to %c" + size_str + " step %c1 {",
      "  %idx = arith.addi %j, %" + index + " : index",
      "  memref.store %c0xFF, %" + access_var_name + "[%idx] : memref<" + size_str + "xi8" + stride_suffix,
      "}",
    };
  }
  return lines;
}

// generate using the given index up to index + distance
std::vector<std::string> DirectLocation::generate_using_runtime_index(
  std::shared_ptr<AccessAction> action,
  const std::string &access_var_name,
  std::string index,
  std::string distance,
  std::function<std::vector<std::string>(const std::string&)> generate_preconditions_check_distance,
  std::function<std::vector<std::string>(const std::string&, const std::string&, const std::string&)> generate_preconditions_check_in_range
) const
{
  std::vector<std::string> lines;
  if (is_a<ReadAction>(action))
  {
    // READ
    lines = {
      "%c0 = arith.constant 0 : index",
      "%c1 = arith.constant 1 : index",
      "scf.for %" + index + " = %c0 to %" + distance + " step %c1 {",
      "  %val = memref.load %" + access_var_name + "[%" + index + "] : memref<8xi8>",
      "}",
    };
  }
  else
  {
    // WRITE
    lines = {
      "%c0 = arith.constant 0 : index",
      "%c1 = arith.constant 1 : index",
      "%c0xFF = arith.constant 255 : i8",
      "scf.for %" + index + " = %c0 to %" + distance + " step %c1 {",
      "  memref.store %c0xFF, %" + access_var_name + "[%" + index + "] : memref<8xi8>",
      "}",
    };
  }
  return lines;
}

// generate in bulks using an auxiliary pointer
AccessLocation::SplitAccess DirectLocation::generate_bulk_split_using_index(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  std::string distance,
  std::function<std::vector<std::string>(const std::string&)>  generate_preconditions_check_distance,
  std::function<std::vector<std::string>(const std::string&, const std::string&, const std::string&)> generate_preconditions_check_in_range,
  std::function<std::vector<std::string>(const std::string&)> generate_counter_update,
  bool needs_strided,
  const std::string &offset
) const
{
  bool is_underflow = false;
  SplitAccess split_access;
  std::vector<std::string> counter_update = generate_counter_update("reach_index");
  for (const auto& line : counter_update)
  {
    if (line.find("subi") != std::string::npos)
    {
      is_underflow = true;
      break;
    }
  }
  std::string index_var = is_underflow ? "index" : "reach_index";
  std::string appendlines = is_underflow ? "  %index = arith.subi %c0, %reach_index : index" : "";
  std::string dist = (is_number(distance) && std::stoll(distance) == 0) ? "c" + distance : distance;
  std::string stride_suffix = needs_strided ? ", strided<[1], offset: " + offset + ">>" : ">";
  if (is_a<ReadAction>(action))
  {
    // READ
    split_access.aux_variables = {
      {"c0", "index", "", "0"},
      {"c1", "index", "", "1"},
    };

    split_access.result = from;
    split_access.access_lines.emplace_back("scf.for %reach_index = %c0 to %" + dist + " step %c1 {");
    if (!appendlines.empty()) split_access.access_lines.emplace_back(appendlines);
    {
      std::string idx_expr = "%" + index_var;
      if (!offset.empty() && offset != "0") {
        split_access.access_lines.emplace_back("  %__base = arith.constant " + offset + " : index");
        split_access.access_lines.emplace_back("  %__idx = arith.addi %" + index_var + ", %__base : index");
        idx_expr = "%__idx";
      }
      split_access.access_lines.emplace_back("  %val = memref.load %" + from + "[" + idx_expr + "] : memref<8xi8" + stride_suffix);
    }
    split_access.access_lines.emplace_back("}");
  }
  else
  {
    // WRITE
    split_access.aux_variables = {
      {"c0", "index", "", "0"},
      {"c1", "index", "", "1"},
      {"c0xFF", "i8", "", "255"},
    };
    split_access.result = from;
    split_access.access_lines.emplace_back("scf.for %reach_index = %c0 to %" + dist + " step %c1 {");
    if (!appendlines.empty()) split_access.access_lines.emplace_back(appendlines);
    {
      std::string idx_expr = "%" + index_var;
      if (!offset.empty() && offset != "0") {
        split_access.access_lines.emplace_back("  %__base = arith.constant " + offset + " : index");
        split_access.access_lines.emplace_back("  %__idx = arith.addi %" + index_var + ", %__base : index");
        idx_expr = "%__idx";
      }
      split_access.access_lines.emplace_back("  memref.store %c0xFF, %" + from + "[" + idx_expr + "] : memref<8xi8" + stride_suffix);
    }
    split_access.access_lines.emplace_back("}");
  }
  split_access.description = "index";
  return split_access;
}

// generate in bulks
AccessLocation::SplitAccess DirectLocation::generate_bulk_split_using_aux_ptr(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  std::string distance,
  std::function<std::vector<std::string>(const std::string&)>  generate_preconditions_check_distance,
  std::function<std::vector<std::string>(const std::string&, const std::string&, const std::string&)>  generate_preconditions_check_in_range,
  std::function<std::vector<std::string>(const std::string&)>  generate_counter_update,
  bool needs_strided,
  const std::string &offset
) const
{
  bool is_underflow = false;
  SplitAccess split_access;
  std::vector<std::string> counter_update = generate_counter_update("reach_index");
  for (const auto& line : counter_update)
  {
    if (line.find("subi") != std::string::npos)
    {
      is_underflow = true;
      break;
    }
  }
  std::string index_var = is_underflow ? "index" : "reach_index";
  std::string appendlines = is_underflow ? "  %index = arith.subi %c0, %reach_index : index" : "";
  std::string dist = (is_number(distance) && std::stoll(distance) == 0) ? "c" + distance : distance;
  std::string stride_suffix = needs_strided ? ", strided<[1], offset: " + offset + ">>" : ">";
  if (is_a<ReadAction>(action))
  {
    // READ
    split_access.aux_variables = {
      {"c0", "index", "", "0"},
      {"c1", "index", "", "1"},
    };

    split_access.result = from;
    split_access.access_lines.emplace_back("scf.for %reach_index = %c0 to %" + dist + " step %c1 {");
    if (!appendlines.empty()) split_access.access_lines.emplace_back(appendlines);
    split_access.access_lines.emplace_back("  %val = memref.load %" + from + "[%" + index_var + "] : memref<8xi8" + stride_suffix);
    split_access.access_lines.emplace_back("}");
  }
  else
  {
    // WRITE
    split_access.aux_variables = {
      {"c0", "index", "", "0"},
      {"c1", "index", "", "1"},
      {"c0xFF", "i8", "", "255"},
    };
    split_access.result = from;
    split_access.access_lines.emplace_back("scf.for %reach_index = %c0 to %" + dist + " step %c1 {");
    if (!appendlines.empty()) split_access.access_lines.emplace_back(appendlines);
    split_access.access_lines.emplace_back("  memref.store %c0xFF, %" + from + "[%" + index_var + "] : memref<8xi8" + stride_suffix);
    split_access.access_lines.emplace_back("}");
  }
  split_access.description = "auxiliary pointer";
  return split_access;
}

// generate using a load widening to uint32
std::vector<std::string> DirectLocation::generate_uint32(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  std::string distance,
  size_t size,
  std::function<std::vector<std::string>(const std::string&)>  generate_preconditions_check_distance,
  bool needs_strided,
  const std::string &offset
) const
{
  std::vector<std::string> lines;
  std::string size_str = std::to_string(size);
  std::string byte_offset = std::to_string(size - 3);
  std::string stride_suffix = needs_strided ? ", strided<[1], offset: " + offset + ">>" : ">";
  if (is_a<ReadAction>(action))
  {
    // READ: manually assemble 4 i8 bytes into i32
    lines = {
      "%c0 = arith.constant 0 : index",
      "%c1 = arith.constant 1 : index",
      "%c2 = arith.constant 2 : index",
      "%c3 = arith.constant 3 : index",
      "%c8_i32  = arith.constant 8 : i32",
      "%c16_i32 = arith.constant 16 : i32",
      "%c24_i32 = arith.constant 24 : i32",
      "%base = arith.constant " + byte_offset + " : index",
      "%b0 = memref.load %" + from + "[%base] : memref<" + size_str + "xi8" + stride_suffix,
      "%idx1 = arith.addi %base, %c1 : index",
      "%b1 = memref.load %" + from + "[%idx1] : memref<" + size_str + "xi8" + stride_suffix,
      "%idx2 = arith.addi %base, %c2 : index",
      "%b2 = memref.load %" + from + "[%idx2] : memref<" + size_str + "xi8" + stride_suffix,
      "%idx3 = arith.addi %base, %c3 : index",
      "%b3 = memref.load %" + from + "[%idx3] : memref<" + size_str + "xi8" + stride_suffix,
      "%b0_i32 = arith.extui %b0 : i8 to i32",
      "%b1_i32 = arith.extui %b1 : i8 to i32",
      "%b2_i32 = arith.extui %b2 : i8 to i32",
      "%b3_i32 = arith.extui %b3 : i8 to i32",
      "%b1_sh = arith.shli %b1_i32, %c8_i32  : i32",
      "%b2_sh = arith.shli %b2_i32, %c16_i32 : i32",
      "%b3_sh = arith.shli %b3_i32, %c24_i32 : i32",
      "%acc0 = arith.ori %b0_i32, %b1_sh : i32",
      "%acc1 = arith.ori %b2_sh, %b3_sh : i32",
      "%val = arith.ori %acc0, %acc1 : i32",
    };
  }
  else
  {
    // WRITE: manually decompose i32 into 4 i8 bytes
    lines = {
      "%c0 = arith.constant 0 : index",
      "%c1 = arith.constant 1 : index",
      "%c2 = arith.constant 2 : index",
      "%c3 = arith.constant 3 : index",
      "%c8_i32  = arith.constant 8 : i32",
      "%c16_i32 = arith.constant 16 : i32",
      "%c24_i32 = arith.constant 24 : i32",
      "%c0xFFFFFFFF = arith.constant 4294967295 : i32",
      "%base = arith.constant " + std::to_string(size - 1) + " : index",
      "%b0 = arith.trunci %c0xFFFFFFFF : i32 to i8",
      "%w1_tmp = arith.shrsi %c0xFFFFFFFF, %c8_i32  : i32",
      "%w2_tmp = arith.shrsi %c0xFFFFFFFF, %c16_i32 : i32",
      "%w3_tmp = arith.shrsi %c0xFFFFFFFF, %c24_i32 : i32",
      "%b1 = arith.trunci %w1_tmp : i32 to i8",
      "%b2 = arith.trunci %w2_tmp : i32 to i8",
      "%b3 = arith.trunci %w3_tmp : i32 to i8",
      "memref.store %b0, %" + from + "[%base] : memref<" + size_str + "xi8" + stride_suffix,
      "%idx1 = arith.addi %base, %c1 : index",
      "memref.store %b1, %" + from + "[%idx1] : memref<" + size_str + "xi8" + stride_suffix,
      "%idx2 = arith.addi %base, %c2 : index",
      "memref.store %b2, %" + from + "[%idx2] : memref<" + size_str + "xi8" + stride_suffix,
      "%idx3 = arith.addi %base, %c3 : index",
      "memref.store %b3, %" + from + "[%idx3] : memref<" + size_str + "xi8" + stride_suffix,
    };
  }
  return lines;
}

// generate after casting to uint8
std::vector<std::string> DirectLocation::generate_uint8(
  std::shared_ptr<AccessAction> action,
  std::string from,
  std::string to,
  std::string distance,
  size_t size,
  std::function<std::vector<std::string>(const std::string&)>  generate_preconditions_check_distance,
  bool needs_strided,
  const std::string &offset
) const
{
  std::vector<std::string> lines;
  std::string size_str = std::to_string(size);
  std::string stride_suffix = needs_strided ? ", strided<[1], offset: " + offset + ">>" : ">";
  if (is_a<ReadAction>(action))
  {
    // READ
    lines = {
      "%idx = arith.constant " + std::to_string(size - 1) + " : index",
      "%val = memref.load %" + from + "[%idx] : memref<" + size_str + "xi8" + stride_suffix,
    };
  }
  else
  {
    // WRITE
    lines = {
      "%idx = arith.constant " + std::to_string(size - 1) + " : index",
      "%c0xFF = arith.constant 255 : i8",
      "memref.store %c0xFF, %" + from + "[%idx] : memref<" + size_str + "xi8" + stride_suffix,
    };
  }
  return lines;
}
