/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "evaluator.h"

#include <algorithm>
#include <cassert>

#include <csignal>   // signal
#include <cstdio>    // remove
#include <cstring>   // strerror
#include <dirent.h>  // opendir, readdir, closedir
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <unistd.h>
#include <set>
#include <sstream>
#include <stddef.h>
#include <sys/stat.h> // stat

#include "config.h"
#include "evaluator/logger.h"
#include "evaluator/sanitizer.h"
#include "evaluator/test_case_information.h"

log_level_t Logger::allowed_log_level = log_level_t::NORMAL;

struct FileInfo
{
  std::string name;
  std::string path;
  bool operator<(const FileInfo& other) const
  {
    return name < other.name;
  }
};

static bool ends_with(const std::string& str, const std::string& suffix)
{
  if (str.size() < suffix.size()) return false;
  return str.substr(str.length() - suffix.length()) == suffix;
}

static bool is_valid_source_file(struct stat file_stat, const std::string &file_name)
{
  return S_ISREG(file_stat.st_mode) && ends_with(file_name, ".c");
}

static bool is_valid_binary_file(struct stat file_stat, const std::string &file_name)
{
  return S_ISREG(file_stat.st_mode) && access(file_name.c_str(), X_OK) == 0;
}

static std::set<FileInfo> parse_dir(const std::string& dir_path, std::function<bool(struct stat, const std::string&)> matcher)
{
  std::set<FileInfo> file_names;
  DIR* dir = opendir(dir_path.c_str());
  if (dir == nullptr)
  {
    std::cerr << "Error opening directory: " << strerror(errno) << '\n';
    exit(EXIT_FAILURE);
  }

  struct dirent* entry;
  while ((entry = readdir(dir)) != nullptr)
  {
    std::string full_path = dir_path + "/" + entry->d_name;
    struct stat file_stat{};
    stat(full_path.c_str(), &file_stat);

    if ( S_ISDIR(file_stat.st_mode) )
    {
      if (entry->d_name[0] == '.')
      {
        continue; // skip "." and ".."
      }
      std::set<FileInfo> files_in_subdir = parse_dir(full_path, matcher);
      for (auto it: files_in_subdir)
      {
        auto res = file_names.insert(it);
        if (!res.second)
        {
          std::cerr << "WARNING: duplicated file in subdir " << full_path << ": " << it.name << '\n';
        }
      }
    }

    if ( matcher(file_stat, full_path) )
    {
      auto res = file_names.insert({entry->d_name, full_path});
      if (!res.second)
      {
        std::cerr << "WARNING: duplicated file: " << entry->d_name << '\n';
      }
    }
  }

  closedir(dir);

  return file_names;
}

static std::set<FileInfo> get_sources_from_dir(const std::string& dir_path)
{
  return parse_dir(dir_path, is_valid_source_file);
}

static std::set<FileInfo> get_binaries_from_dir(const std::string& dir_path)
{
  return parse_dir(dir_path, is_valid_binary_file);
}

static std::vector<exec_result_t> raw_overall_results;
static std::vector<exec_result_t> raw_overall_baseline_results;
static std::map<std::string, std::vector<exec_result_t>> raw_temporal_results;
static std::map<std::string, std::vector<exec_result_t>> raw_spatial_results;
static std::map<std::string, std::vector<exec_result_t>> raw_temporal_baseline_results;
static std::map<std::string, std::vector<exec_result_t>> raw_spatial_baseline_results;
static std::map<std::shared_ptr<TemporalTestCaseInformation>, std::vector<exec_result_t>, CompareTestCaseInformation> temporal_results;
static std::map<std::shared_ptr<SpatialTestCaseInformation>, std::vector<exec_result_t>, CompareTestCaseInformation> spatial_results;
static std::map<std::shared_ptr<TemporalTestCaseInformation>, std::vector<exec_result_t>, CompareTestCaseInformation> temporal_baseline_results;
static std::map<std::shared_ptr<SpatialTestCaseInformation>, std::vector<exec_result_t>, CompareTestCaseInformation> spatial_baseline_results;

static bool collect_result(const std::shared_ptr<TestCaseInformation> &test_case_information, exec_result_t result, const std::string &file_name, bool is_baseline)
{
  bool can_stop = false;
  if (is_baseline) Logger(log_level_t::VERBOSE) << "Baseline result for ";
  else Logger(log_level_t::VERBOSE) << "Result for ";
  Logger(log_level_t::VERBOSE) << test_case_information->to_string() << " (" << file_name << "): ";

  switch (result)
  {
    case PRECONDITIONS_FAILED:
      Logger(log_level_t::VERBOSE) << "PRECONDITIONS_FAILED\n";
      break;
    case FAILED:
      Logger(log_level_t::VERBOSE) << "FAILED\n";
      break;
    case FAILED_SIGSEGV:
      Logger(log_level_t::VERBOSE) << "FAILED_SIGSEGV\n";
      break;
    case TIMEOUT:
      Logger(log_level_t::VERBOSE) << "TIMEOUT\n";
      break;
    case SUCCESSFUL:
      Logger(log_level_t::VERBOSE) << "SUCCESSFUL\n";
      can_stop = true;
      break;
    default:
      break;
  }

  const std::shared_ptr<TemporalTestCaseInformation> temporal_ptr = std::dynamic_pointer_cast<TemporalTestCaseInformation>(test_case_information);
  if ( temporal_ptr )
  {
    if (is_baseline) temporal_baseline_results[temporal_ptr].push_back(result);
    else temporal_results[temporal_ptr].push_back(result);
  }
  else
  {
    const std::shared_ptr<SpatialTestCaseInformation> spatial_ptr = std::dynamic_pointer_cast<SpatialTestCaseInformation>(test_case_information);
    assert(spatial_ptr);
    if (is_baseline) spatial_baseline_results[spatial_ptr].push_back(result);
    else spatial_results[spatial_ptr].push_back(result);
  }
  return can_stop;
}

static bool collect_validation_result(const std::shared_ptr<TestCaseInformation> &test_case_information, exec_result_t result, const std::string &file_name)
{
  bool valid = false;
  Logger(log_level_t::VERBOSE) << "Validation result for " << test_case_information->to_string() << " (" << file_name << "): ";
  switch (result)
  {
    case PRECONDITIONS_FAILED:
      Logger(log_level_t::VERBOSE) << "PRECONDITIONS_FAILED\n";
      break;
    case FAILED:
      Logger(log_level_t::VERBOSE) << "FAILED\n";
      break;
    case FAILED_SIGSEGV:
      Logger(log_level_t::VERBOSE) << "FAILED_SIGSEGV\n";
      break;
    case TIMEOUT:
      Logger(log_level_t::VERBOSE) << "TIMEOUT\n";
      break;
    case SUCCESSFUL:
      Logger(log_level_t::VERBOSE) << "SUCCESSFUL\n";
      valid = true;
      break;
    default:
      break;
  }

  if (valid)
  {
    return false; // continue validating
  }


  const std::shared_ptr<TemporalTestCaseInformation> temporal_ptr = std::dynamic_pointer_cast<TemporalTestCaseInformation>(test_case_information);
  if ( temporal_ptr )
  {
    temporal_results[temporal_ptr].push_back(INVALID);
  }
  else
  {
    const std::shared_ptr<SpatialTestCaseInformation> spatial_ptr = std::dynamic_pointer_cast<SpatialTestCaseInformation>(test_case_information);
    assert(spatial_ptr);
    spatial_results[spatial_ptr].push_back(INVALID);
  }
  return true;
}

static exec_result_t compute_overall_result( const std::vector<exec_result_t> &results )
{
  exec_result_t overall_result;
  size_t precond_failed = 0;
  size_t failures = 0;
  size_t successes = 0;
  size_t validation_failures = 0;
  for ( const auto result: results )
  {
    if ( result == INVALID ) validation_failures++;
    else if ( result == PRECONDITIONS_FAILED ) precond_failed++;
    else if ( result == SUCCESSFUL || result == TIMEOUT ) successes++;
    else failures++;
  }
  (void)precond_failed;
  if ( validation_failures > 0 )
  { // if any variant is invalid
    overall_result = INVALID;
  }
  else if ( successes > 0 )
  { // if any variant is successful
    overall_result = SUCCESSFUL;
  }
  else if ( failures > 0 )
  {
    overall_result = FAILED;
  }
  else
  { // only if all variants have failing preconditions
    overall_result = PRECONDITIONS_FAILED;
  }

  return overall_result;
}

static void print_overall_result(exec_result_t overall_result, const std::string& test_info_string)
{
  Logger(log_level_t::NORMAL) << "For " << test_info_string << ", the overall result is " << exec_result_to_string(overall_result) << ".\n";
}

static void print_overall_result_with_baseline(exec_result_t overall_result, exec_result_t overall_baseline_result, const std::string& test_info_string)
{
  Logger(log_level_t::NORMAL) << "For " << test_info_string << ", the overall result is " << exec_result_to_string(overall_result) << " (Baseline: " << exec_result_to_string(overall_baseline_result) << ").\n";
}

static void collapse_results(bool compute_baseline)
{
  for ( auto key_val: temporal_results )
  {
    std::shared_ptr<TemporalTestCaseInformation> compressed_result_info = std::dynamic_pointer_cast<TemporalTestCaseInformation>(std::get<0>(key_val));
    std::vector<exec_result_t> results = std::get<1>(key_val);

    exec_result_t overall_result = compute_overall_result( results );

    raw_temporal_results[compressed_result_info->temporal_bug_name].push_back(overall_result);
    raw_temporal_results[compressed_result_info->temporal_memory_state_name].push_back(overall_result);
    raw_temporal_results[compressed_result_info->region_name].push_back(overall_result);
    raw_temporal_results[compressed_result_info->access_location_name].push_back(overall_result);
    raw_temporal_results[compressed_result_info->access_action_name].push_back(overall_result);
    raw_temporal_results["all"].push_back(overall_result);
    raw_overall_results.push_back(overall_result);

    if ( compute_baseline )
    {
      assert(temporal_baseline_results.find(compressed_result_info) != temporal_baseline_results.end());
      std::vector<exec_result_t> baseline_results = temporal_baseline_results.at(compressed_result_info);

      exec_result_t overall_baseline_result = compute_overall_result( baseline_results );

      raw_temporal_baseline_results[compressed_result_info->temporal_bug_name].push_back(overall_baseline_result);
      raw_temporal_baseline_results[compressed_result_info->temporal_memory_state_name].push_back(overall_baseline_result);
      raw_temporal_baseline_results[compressed_result_info->region_name].push_back(overall_baseline_result);
      raw_temporal_baseline_results[compressed_result_info->access_location_name].push_back(overall_baseline_result);
      raw_temporal_baseline_results[compressed_result_info->access_action_name].push_back(overall_baseline_result);
      raw_temporal_baseline_results["all"].push_back(overall_baseline_result);
      raw_overall_baseline_results.push_back(overall_baseline_result);

      print_overall_result_with_baseline(overall_result, overall_baseline_result, compressed_result_info->get_test_case_key());

    }
    else
    {
      print_overall_result(overall_result, compressed_result_info->get_test_case_key());
    }
  }


  for ( auto key_val: spatial_results )
  {
    std::shared_ptr<SpatialTestCaseInformation> compressed_result_info = std::get<0>(key_val);
    std::vector<exec_result_t> results = std::get<1>(key_val);

    exec_result_t overall_result = compute_overall_result( results );

    raw_spatial_results[compressed_result_info->origin_name].push_back(overall_result);
    raw_spatial_results[compressed_result_info->target_name].push_back(overall_result);
    raw_spatial_results[compressed_result_info->origin_target_relation_name].push_back(overall_result);
    raw_spatial_results[compressed_result_info->flow_name].push_back(overall_result);
    raw_spatial_results[compressed_result_info->spatial_bug_name].push_back(overall_result);
    raw_spatial_results[compressed_result_info->access_location_name].push_back(overall_result);
    raw_spatial_results[compressed_result_info->access_action_name].push_back(overall_result);
    raw_spatial_results["all"].push_back(overall_result);
    raw_overall_results.push_back(overall_result);

    if ( compute_baseline )
    {
      assert(spatial_baseline_results.find(compressed_result_info) != spatial_baseline_results.end());
      std::vector<exec_result_t> baseline_results = spatial_baseline_results.at(compressed_result_info);

      exec_result_t overall_baseline_result = compute_overall_result( baseline_results );

      raw_spatial_baseline_results[compressed_result_info->origin_name].push_back(overall_baseline_result);
      raw_spatial_baseline_results[compressed_result_info->target_name].push_back(overall_baseline_result);
      raw_spatial_baseline_results[compressed_result_info->origin_target_relation_name].push_back(overall_baseline_result);
      raw_spatial_baseline_results[compressed_result_info->flow_name].push_back(overall_baseline_result);
      raw_spatial_baseline_results[compressed_result_info->spatial_bug_name].push_back(overall_baseline_result);
      raw_spatial_baseline_results[compressed_result_info->access_location_name].push_back(overall_baseline_result);
      raw_spatial_baseline_results[compressed_result_info->access_action_name].push_back(overall_baseline_result);
      raw_spatial_baseline_results["all"].push_back(overall_baseline_result);
      print_overall_result_with_baseline(overall_result, overall_baseline_result, compressed_result_info->get_test_case_key());

      raw_overall_baseline_results.push_back(overall_baseline_result);
    }
    else
    {
      print_overall_result(overall_result, compressed_result_info->get_test_case_key());
    }
  }
}

std::string score_to_str(const double value)
{
  std::ostringstream percentage;
  percentage << std::fixed;
  percentage << std::setprecision(2);
  percentage << value;
  return percentage.str() + "%";
}

struct counters_t
{
  size_t precond_failed = 0;
  size_t failures = 0;
  size_t successes = 0;
  size_t invalids = 0;
};

counters_t compute_counters(const std::vector<exec_result_t> &raw_results)
{
  counters_t counters;
  for ( auto result: raw_results )
  {
    if ( result == INVALID ) counters.invalids++;
    else if ( result == PRECONDITIONS_FAILED ) counters.precond_failed++;
    else if ( result == SUCCESSFUL || result == TIMEOUT ) counters.successes++;
    else counters.failures++;
  }
  return counters;
}


static void print_results(const std::vector<exec_result_t> &raw_results, const std::vector<exec_result_t> &baseline_results, const log_level_t log_level)
{
  std::string overall_detection_rate;
  std::string precond_failed_percentage;
  std::string failures_percentage;
  std::string successes_percentage;
  counters_t counters = compute_counters(raw_results);
  if ( raw_results.empty() )
  {
    overall_detection_rate = "N/A";
    precond_failed_percentage = "N/A";
    failures_percentage = "N/A";
    successes_percentage = "N/A";
  }
  else
  {
    overall_detection_rate = score_to_str( (static_cast<double>(counters.precond_failed + counters.failures) * 100) / static_cast<double>(raw_results.size()) );
    precond_failed_percentage = score_to_str( (static_cast<double>(counters.precond_failed) * 100) / static_cast<double>(raw_results.size()) );
    failures_percentage = score_to_str( (static_cast<double>(counters.failures) * 100) / static_cast<double>(raw_results.size()) );
    successes_percentage = score_to_str( (static_cast<double>(counters.successes + counters.invalids) * 100) / static_cast<double>(raw_results.size()) );
  }

  bool with_baseline = !baseline_results.empty();

  std::string overall_baseline_detection_rate;
  std::string precond_failed_baseline_percentage;
  std::string failures_baseline_percentage;
  std::string successes_baseline_percentage;
  counters_t baseline_counters;

  if (with_baseline)
  {
    baseline_counters = compute_counters(baseline_results);
    if ( baseline_results.empty() )
    {
      overall_baseline_detection_rate = "N/A";
      precond_failed_baseline_percentage = "N/A";
      failures_baseline_percentage = "N/A";
      successes_baseline_percentage = "N/A";
    }
    else
    {
      overall_baseline_detection_rate = score_to_str( (static_cast<double>(baseline_counters.precond_failed + baseline_counters.failures) * 100) / static_cast<double>(baseline_results.size()) );
      precond_failed_baseline_percentage = score_to_str( (static_cast<double>(baseline_counters.precond_failed) * 100) / static_cast<double>(baseline_results.size()) );
      failures_baseline_percentage = score_to_str( (static_cast<double>(baseline_counters.failures) * 100) / static_cast<double>(baseline_results.size()) );
      successes_baseline_percentage = score_to_str( (static_cast<double>(baseline_counters.successes + baseline_counters.invalids) * 100) / static_cast<double>(baseline_results.size()) );
    }
  }

  Logger(log_level) << "Detection rate: " << overall_detection_rate << " (" << counters.precond_failed + counters.failures << " out of " << raw_results.size() << " test cases)";
  if (with_baseline) Logger(log_level) << " / Baseline: " << overall_baseline_detection_rate;
  Logger(log_level) << "\n";
  Logger(log_level) << "Results for test cases:\n";
  Logger(log_level) << "- Preconditions failed: " << precond_failed_percentage << " (" << counters.precond_failed << ")";
  if (with_baseline) Logger(log_level) << " / Baseline: " << precond_failed_baseline_percentage;
  Logger(log_level) << "\n";

  Logger(log_level) << "- Failures: " << failures_percentage << " (" << counters.failures << ")";
  if (with_baseline) Logger(log_level) << " / Baseline: " << failures_baseline_percentage;
  Logger(log_level) << "\n";

  assert(baseline_counters.invalids == 0);
  if ( counters.invalids == 0 )
  {
    Logger(log_level) << "- Successes: " << successes_percentage << " (" << counters.successes << ")";
    if (with_baseline) Logger(log_level) << " / Baseline: " << successes_baseline_percentage;
    Logger(log_level) << "\n";
  }
  else
  {
    Logger(log_level) << "- Successes: " << successes_percentage << " (" << counters.successes << " successful attempts, " << counters.invalids << " didn't pass validation)";
    if (with_baseline) Logger(log_level) << " / Baseline: " << successes_baseline_percentage;
    Logger(log_level) << "\n";
  }
}

static void process_results(bool print_table_summary, bool with_baseline)
{
  Logger(log_level_t::VERBOSE) << "==============================\n\n";

  Logger(log_level_t::VERBOSE) << "Temporal bugs:\n";
  print_results(raw_temporal_results["all"], raw_temporal_baseline_results["all"], log_level_t::VERBOSE);

  Logger(log_level_t::VERBOSE) << "Bug detection distribution per bug type:\n";
  for ( const auto& info: temporal_bugs_info )
  {
    Logger(log_level_t::VERBOSE) << info << ":\n";
    print_results(raw_temporal_results[info], raw_temporal_baseline_results[info], log_level_t::VERBOSE);
  }
  Logger(log_level_t::VERBOSE) << "Bug detection distribution per region:\n";
  for ( const auto& info: regions_info )
  {
    Logger(log_level_t::VERBOSE) << info << ":\n";
    print_results(raw_temporal_results[info], raw_temporal_baseline_results[info], log_level_t::VERBOSE);
  }
  Logger(log_level_t::VERBOSE) << "Bug detection distribution per memory state:\n";
  for ( const auto& info: temporal_memory_states_info )
  {
    Logger(log_level_t::VERBOSE) << info << ":\n";
    print_results(raw_temporal_results[info], raw_temporal_baseline_results[info], log_level_t::VERBOSE);
  }
  Logger(log_level_t::VERBOSE) << "Bug detection distribution per access type location:\n";
  for ( const auto& info: access_locations_info )
  {
    Logger(log_level_t::VERBOSE) << info << ":\n";
    print_results(raw_temporal_results[info], raw_temporal_baseline_results[info], log_level_t::VERBOSE);
  }
  Logger(log_level_t::VERBOSE) << "Bug detection distribution per access type action:\n";
  for ( const auto& info: access_actions_info )
  {
    Logger(log_level_t::VERBOSE) << info << ":\n";
    print_results(raw_temporal_results[info], raw_temporal_baseline_results[info], log_level_t::VERBOSE);
  }

  Logger(log_level_t::VERBOSE) << "==============================\n\n";
  Logger(log_level_t::VERBOSE) << "Spatial bugs:\n";
  print_results(raw_spatial_results["all"], raw_spatial_baseline_results["all"], log_level_t::VERBOSE);

  Logger(log_level_t::VERBOSE) << "Bug detection distribution per bug type:\n";
  for ( const auto& info: spatial_bugs_info )
  {
    Logger(log_level_t::VERBOSE) << info << ":\n";
    print_results(raw_spatial_results[info], raw_spatial_baseline_results[info], log_level_t::VERBOSE);
  }
  Logger(log_level_t::VERBOSE) << "Bug detection distribution per origin:\n";
  for ( const auto& info: regions_info )
  {
    Logger(log_level_t::VERBOSE) << info << ":\n";
    print_results(raw_spatial_results[info], raw_spatial_baseline_results[info], log_level_t::VERBOSE);
  }
  Logger(log_level_t::VERBOSE) << "Bug detection distribution per target:\n";
  for ( const auto& info: regions_info )
  {
    Logger(log_level_t::VERBOSE) << info << ":\n";
    print_results(raw_spatial_results[info], raw_spatial_baseline_results[info], log_level_t::VERBOSE);
  }
  Logger(log_level_t::VERBOSE) << "Bug detection distribution per origin-target relation:\n";
  for ( const auto& info: origin_target_relations_info )
  {
    Logger(log_level_t::VERBOSE) << info << ":\n";
    print_results(raw_spatial_results[info], raw_spatial_baseline_results[info], log_level_t::VERBOSE);
  }
  Logger(log_level_t::VERBOSE) << "Bug detection distribution per flow:\n";
  for ( const auto& info: flows_info )
  {
    Logger(log_level_t::VERBOSE) << info << ":\n";
    print_results(raw_spatial_results[info], raw_spatial_baseline_results[info], log_level_t::VERBOSE);
  }
  Logger(log_level_t::VERBOSE) << "Bug detection distribution per access type location:\n";
  for ( const auto& info: access_locations_info )
  {
    Logger(log_level_t::VERBOSE) << info << ":\n";
    print_results(raw_spatial_results[info], raw_spatial_baseline_results[info], log_level_t::VERBOSE);
  }
  Logger(log_level_t::VERBOSE) << "Bug detection distribution per access type action:\n";
  for ( const auto& info: access_actions_info )
  {
    Logger(log_level_t::VERBOSE) << info << ":\n";
    print_results(raw_spatial_results[info], raw_spatial_baseline_results[info], log_level_t::VERBOSE);
  }

  Logger(log_level_t::VERBOSE) << "==============================\n\n";
  Logger(log_level_t::NORMAL) << "Overall results:\n";
  print_results(raw_overall_results, raw_overall_baseline_results, log_level_t::NORMAL);

  if (print_table_summary)
  {
    std::cout << "Prevented summary:\n";
    std::cout << std::left << std::setw(12) << "Linear OOBA" << "| " << std::setw(16) << "Non-Linear OOBA " << "| " <<
      std::setw(20) << "Type Confusion OOBA" << "| " << std::setw(12) << "Use-after-*" << "| " <<
      std::setw(12) << "Double-free" << "| " << std::setw(14) << "Misuse-of-free\n";
    counters_t counters = compute_counters(raw_spatial_results["Linear OOBA"]);
    std::cout << std::left << std::setw(12) << counters.precond_failed + counters.failures << "| ";
    counters = compute_counters(raw_spatial_results["Non-Linear OOBA"]);
    std::cout << std::setw(16) << counters.precond_failed + counters.failures << "| ";
    counters = compute_counters(raw_spatial_results["Type Confusion OOBA"]);
    std::cout << std::setw(20) << counters.precond_failed + counters.failures << "| ";
    counters = compute_counters(raw_temporal_results["Use-after-*"]);
    std::cout << std::setw(12) << counters.precond_failed + counters.failures << "| ";
    counters = compute_counters(raw_temporal_results["Double-free"]);
    std::cout << std::setw(12) << counters.precond_failed + counters.failures << "| ";
    counters = compute_counters(raw_temporal_results["Misuse-of-free"]);
    std::cout << std::setw(14) << counters.precond_failed + counters.failures << "\n";
    if (with_baseline)
    {
      counters = compute_counters(raw_spatial_baseline_results["Linear OOBA"]);
      std::cout << std::left << std::setw(12) << counters.precond_failed + counters.failures << "| ";
      counters = compute_counters(raw_spatial_baseline_results["Non-Linear OOBA"]);
      std::cout << std::setw(16) << counters.precond_failed + counters.failures << "| ";
      counters = compute_counters(raw_spatial_baseline_results["Type Confusion OOBA"]);
      std::cout << std::setw(20) << counters.precond_failed + counters.failures << "| ";
      counters = compute_counters(raw_temporal_baseline_results["Use-after-*"]);
      std::cout << std::setw(12) << counters.precond_failed + counters.failures << "| ";
      counters = compute_counters(raw_temporal_baseline_results["Double-free"]);
      std::cout << std::setw(12) << counters.precond_failed + counters.failures << "| ";
      counters = compute_counters(raw_temporal_baseline_results["Misuse-of-free"]);
      std::cout << std::setw(14) << counters.precond_failed + counters.failures << "  (baseline)\n";
    }
  }
}

extern void compile_and_evaluate(
  const std::string &test_cases_dir_path,
  const std::string &sanitizer_config,
  bool print_table_summary,
  bool run_all_variants,
  bool verbose,
  bool compute_baseline,
  bool keep_binaries
)
{
  if (verbose)
  {
    Logger::allowed_log_level = log_level_t::VERBOSE;
  }
  size_t variant_eval_counter = 0;
  Sanitizer sanitizer{sanitizer_config};

  const std::set<FileInfo> generated_files = get_sources_from_dir(test_cases_dir_path);

  if (generated_files.empty())
  {
    std::cerr << "ERROR: No test files found. Aborting.\n";
    exit(EXIT_FAILURE);
  }

  std::map< std::string, std::vector<std::shared_ptr<TestCaseInformation>> > grouped_test_cases;
  for (const auto& file_path : generated_files)
  {
    std::shared_ptr<TestCaseInformation> test_case_information = TestCaseInformation::construct_from_file_name(file_path.name, file_path.path, /*is_binary=*/false);
    grouped_test_cases[test_case_information->get_test_case_key()].push_back(test_case_information);
  }
  for (auto& grouped_test_case : grouped_test_cases)
  {
    std::vector<std::shared_ptr<TestCaseInformation>> &test_case_infos = grouped_test_case.second;
    std::sort(test_case_infos.begin(), test_case_infos.end(), compare_test_case_variants);

    Logger(log_level_t::VERBOSE) << "Evaluating baseline: " << grouped_test_case.first << "\n";
    if (compute_baseline)
    {
      for (const auto& test_case_info : test_case_infos)
      {
        if ( test_case_info->get_is_validation() ) continue; // only normal phase for the baseline

        std::string dir_path = test_cases_dir_path;
        if (dir_path.back() != '/')
        {
          dir_path += "/";
        }
        std::string binary_path = dir_path + "/" + TEST_CASE_BINARIES_DIR_NAME + "/" + test_case_info->get_file_name_without_suffix() + "_baseline";

        if ( !sanitizer.compile_baseline(test_case_info->get_file_path(), binary_path) )
        {
          std::cerr << "Failed to compile baseline " << test_case_info->get_file_path() << '\n';
          std::cerr << "Aborting.\n";
          exit(EXIT_FAILURE);
        }
        exec_result_t result = sanitizer.execute_baseline(binary_path);

        if (!keep_binaries)
        {
          remove(binary_path.c_str());
        }

        bool can_stop = collect_result(test_case_info, result, test_case_info->get_file_name(), /*is_baseline=*/true);
        if (can_stop && !run_all_variants)
        {
          break;
        }
      }
    }

    Logger(log_level_t::NORMAL) << "Evaluating: " << grouped_test_case.first << "\n";
    for (const auto& test_case_info : test_case_infos)
    {
      std::string dir_path = test_cases_dir_path;
      if (dir_path.back() != '/')
      {
        dir_path += "/";
      }
      std::string binary_path = dir_path + "/" + TEST_CASE_BINARIES_DIR_NAME + "/" + test_case_info->get_file_name_without_suffix();

      if ( !sanitizer.compile(test_case_info->get_file_path(), binary_path) )
      {
        std::cerr << "Failed to compile " << test_case_info->get_file_path() << '\n';
        std::cerr << "Aborting.\n";
        exit(EXIT_FAILURE);
      }
      exec_result_t result = sanitizer.execute(binary_path);

      if (!keep_binaries)
      {
        remove(binary_path.c_str());
      }

      bool can_stop;
      if ( test_case_info->get_is_validation() )
      {
        // validation phase
        can_stop = collect_validation_result(test_case_info, result, test_case_info->get_file_name());
      }
      else
      {
        // normal phase
        can_stop = collect_result(test_case_info, result, test_case_info->get_file_name(), /*is_baseline=*/false);
        variant_eval_counter++;
      }
      if (can_stop && !run_all_variants)
      {
        break;
      }
    }
  }

  Logger(log_level_t::NORMAL) << "Evaluated " << grouped_test_cases.size() << " test cases, " << variant_eval_counter << " variants.\n";
  collapse_results(compute_baseline);
  process_results(print_table_summary, compute_baseline);
}

extern void evaluate_prebuilt_binaries(
  const std::string &test_cases_dir_path,
  const std::string &sanitizer_config,
  bool print_table_summary,
  bool run_all_variants,
  bool verbose,
  bool compute_baseline
)
{
  if (verbose)
  {
    Logger::allowed_log_level = log_level_t::VERBOSE;
  }
  size_t variant_eval_counter = 0;
  Sanitizer sanitizer{sanitizer_config};

  const std::set<FileInfo> binary_files = get_binaries_from_dir(test_cases_dir_path);

  if (binary_files.empty())
  {
    std::cerr << "ERROR: No test files found. Aborting.\n";
    exit(EXIT_FAILURE);
  }

  std::map< std::string, std::vector<std::shared_ptr<TestCaseInformation>> > grouped_test_cases;
  for (const auto& binary_path : binary_files)
  {
    std::shared_ptr<TestCaseInformation> test_case_information = TestCaseInformation::construct_from_file_name(binary_path.name, binary_path.path, /*is_binary=*/true);
    grouped_test_cases[test_case_information->get_test_case_key()].push_back(test_case_information);
  }
  for (auto& grouped_test_case : grouped_test_cases)
  {
    std::vector<std::shared_ptr<TestCaseInformation>> &test_case_infos = grouped_test_case.second;
    std::sort(test_case_infos.begin(), test_case_infos.end(), compare_test_case_variants);

    if (compute_baseline)
    {
      Logger(log_level_t::VERBOSE) << "Evaluating baseline: " << grouped_test_case.first << "\n";
      for (const auto& test_case_info : test_case_infos)
      {
        if ( test_case_info->get_is_validation() // only normal phase for the baseline
          || !ends_with(test_case_info->get_file_name(), "_baseline")  // binary name must end with _baseline
          )
        {
          continue;
        }

        std::string binary_path = test_case_info->get_file_path();

        exec_result_t result = sanitizer.execute_baseline(binary_path);

        bool can_stop = collect_result(test_case_info, result, test_case_info->get_file_name(), /*is_baseline=*/true);
        if (can_stop && !run_all_variants)
        {
          break;
        }
      }
    }

    Logger(log_level_t::NORMAL) << "Evaluating: " << grouped_test_case.first << "\n";
    for (const auto& test_case_info : test_case_infos)
    {
      std::string binary_path = test_case_info->get_file_path();
      exec_result_t result = sanitizer.execute(binary_path);

      bool can_stop;
      if ( test_case_info->get_is_validation() )
      {
        // validation phase
        can_stop = collect_validation_result(test_case_info, result, test_case_info->get_file_name());
      }
      else
      {
        // normal phase
        can_stop = collect_result(test_case_info, result, test_case_info->get_file_name(), /*is_baseline=*/false);
        variant_eval_counter++;
      }
      if (can_stop && !run_all_variants)
      {
        break;
      }
    }
  }

  Logger(log_level_t::NORMAL) << "Evaluated " << grouped_test_cases.size() << " test cases, " << variant_eval_counter << " variants.\n";
  collapse_results(compute_baseline);
  process_results(print_table_summary, compute_baseline);
}

extern void compile_all(
  const std::string &generated_path,
  const std::string &sanitizer_config,
  bool verbose
)
{
  if ( verbose )
  {
    Logger::allowed_log_level = log_level_t::VERBOSE;
  }

  size_t total_counter = 0;
  size_t validation_counter = 0;
  size_t normal_counter = 0;
  Sanitizer sanitizer{sanitizer_config};

  const std::set<FileInfo> generated_files = get_sources_from_dir(generated_path);

  if ( generated_files.empty() )
  {
    std::cerr << "ERROR: No test files found. Aborting.\n";
    exit(EXIT_FAILURE);
  }

  std::string dir_path = generated_path + "/" + TEST_CASE_BINARIES_DIR_NAME;

  for ( auto &generated_file : generated_files )
  {
    std::shared_ptr<TestCaseInformation> test_case_information = TestCaseInformation::construct_from_file_name(
      generated_file.name, generated_file.path, /*is_binary=*/false);

    std::string binary_path = dir_path + "/" + test_case_information->get_file_name_without_suffix();

    Logger(log_level_t::VERBOSE) << "Compiling: " << generated_file.name << " -> " << binary_path << "\n";
    if ( !sanitizer.compile( test_case_information->get_file_path(), binary_path ) )
    {
      std::cerr << "Failed to compile baseline " << test_case_information->get_file_path() << '\n';
      std::cerr << "Aborting.\n";
      exit(EXIT_FAILURE);
    }
    total_counter++;

    if ( !test_case_information->get_is_validation() )  // don't validate baselines
    {
      total_counter++;
      normal_counter++;
      Logger(log_level_t::VERBOSE) << "Compiling baseline for: " << generated_file.name << "\n";

      binary_path += "_baseline";

      if ( !sanitizer.compile_baseline( test_case_information->get_file_path(), binary_path ) )
      {
        std::cerr << "Failed to compile baseline " << test_case_information->get_file_path() << '\n';
        std::cerr << "Aborting.\n";
        exit(EXIT_FAILURE);
      }
    }
    else
    {
      validation_counter++;
    }
  }

  Logger(log_level_t::NORMAL) << "Compiled " << total_counter << " files: " << normal_counter << " normal test cases, "
    << normal_counter << " baselines test cases and " << validation_counter << " validation test cases.\n";
}