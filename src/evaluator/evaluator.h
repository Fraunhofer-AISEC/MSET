/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include <string>

extern void evaluate(
  const std::string &generated_path,
  const std::string &sanitizer_config,
  bool print_table_summary,
  bool run_all_variants,
  bool verbose,
  bool compute_baseline,
  bool keep_binaries
);
