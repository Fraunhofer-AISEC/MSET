/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include <string>

extern void compile_and_evaluate(
  const std::string &test_cases_dir_path,
  const std::string &sanitizer_config,
  bool print_table_summary,
  bool run_all_variants,
  bool verbose,
  bool compute_baseline,
  bool keep_binaries
);

extern void evaluate_prebuilt_binaries(
  const std::string &test_cases_dir_path,
  const std::string &sanitizer_config,
  bool print_table_summary,
  bool run_all_variants,
  bool verbose,
  bool compute_baseline
);

extern void compile_all(
  const std::string &generated_path,
  const std::string &sanitizer_config,
  bool verbose
);
