/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <tuple>

enum exec_result_t
{
  PRECONDITIONS_FAILED,
  FAILED,
  FAILED_SIGSEGV,
  TIMEOUT,
  INVALID,
  SUCCESSFUL
};

inline std::string exec_result_to_string(const exec_result_t result)
{
  switch( result )
  {
    case PRECONDITIONS_FAILED:
      return "PRECONDITIONS FAILED";
    case FAILED:
      return "FAILED";
    case INVALID:
      return "INVALID";
    case SUCCESSFUL:
      return "SUCCESSFUL";
    case FAILED_SIGSEGV:
    case TIMEOUT:
    default:
      std::cerr << "ERROR: UNKNOWN.\n";
      exit(EXIT_FAILURE);
  }
}

class Sanitizer
{
public:
  explicit Sanitizer(const std::string &config_path);

  bool compile(const std::string &src_file_path, const std::string &binary_path) const;
  exec_result_t execute(const std::string &binary_path) const;

  bool compile_baseline(const std::string &src_file_path, const std::string &binary_path) const;
  exec_result_t execute_baseline(const std::string &binary_path) const;
private:
  std::vector<std::string> setup_commands;
  std::string compile_command;
  std::vector<std::string> baseline_setup_commands;
  std::string baseline_compile_command;
  std::string execute_command;
  std::vector< std::tuple<std::string, std::string> > exec_env_vars;
  std::string baseline_execute_command;
  std::string sanitizer_name;

  int test_case_successful_exit_value;
  std::set<int> test_case_failed_exit_values;
  int preconditions_not_met_exit_value;
  int timeout_exit_value;
  int timeout_in_secs;

  std::string defines;

  bool _compile(const std::string &src_file_path, const std::string &resulted_binary_path, const std::string &compile_command, const std::vector<std::string> &commands) const;

  exec_result_t _execute(
    const std::string &binary_path,
    const std::vector<std::string> &binary_args,
    const std::vector< std::tuple<std::string, std::string> > &env_vars,
    int timeout_s) const;
};
