/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once
#include <memory>
#include <string>
#include <vector>

const std::vector<std::string> regions ={
  "heap",
  "stack",
  "global"
};
const std::vector<std::string> regions_info ={
  {"Heap"},
  {"Stack"},
  {"Global"}
};


const std::vector<std::string> temporal_bug_types ={
  "misuse_of_free",
  "double_free",
  "use_after_star"
};
const std::vector<std::string> temporal_bugs_info ={
  {"Misuse-of-free"},
  {"Double-free"},
  {"Use-after-*"}
};


const std::vector<std::string> temporal_memory_states ={
  "used_memory",
  "freed_memory",
};
const std::vector<std::string> temporal_memory_states_info ={
  {"(Re)used Memory"},
  {"Freed Memory"}
};


const std::vector<std::string> spatial_bug_types ={
  "linear_ooba",
  "non_linear_ooba",
  "type_confusion_ooba"
};
const std::vector<std::string> spatial_bugs_info ={
  {"Linear OOBA"},
  {"Non-Linear OOBA"},
  {"Type Confusion OOBA"}
};


const std::vector<std::string> origin_target_relations ={
  "non_object",
  "intra_object",
  "inter_object",
};
const std::vector<std::string> origin_target_relations_info ={
  {"Non-Object"},
  {"Intra-Object"},
  {"Inter-Object"},
};


const std::vector<std::string> flows ={
  "overflow",
  "underflow",
};
const std::vector<std::string> flows_info ={
  {"Overflow"},
  {"Underflow"}
};


const std::vector<std::string> access_locations ={
  "stdlib",
  "direct",
};
const std::vector<std::string> access_locations_info ={
  {"Stdlib"},
  {"Direct"}
};


const std::vector<std::string> access_actions ={
  "read",
  "write",
};
const std::vector<std::string> access_actions_info ={
  {"Read"},
  {"Write"}
};

class TestCaseInformation
{
public:
  virtual ~TestCaseInformation() = default;
  std::string to_string() const { return as_string; }
  std::string get_test_case_key() const { return key; }
  std::string get_file_name() const { return file_name; }
  std::string get_file_name_without_suffix() const { return file_name_without_suffix; }
  bool get_is_validation() const { return is_validation; }
  int get_variant_number() const { return variant_number; }

  static std::shared_ptr<TestCaseInformation> construct_from_file_name(const std::string &file_name);

protected:
  TestCaseInformation(bool is_validation, int variant_number, const std::string &file_name, const std::string &file_name_without_suffix);

  std::string file_name;
  std::string file_name_without_suffix;
  bool is_validation;
  int variant_number;
  std::string as_string;
  std::string key; /* same for all variants */
};


struct CompareTestCaseInformation
{
  bool operator()(const std::shared_ptr<TestCaseInformation>& lhs, const std::shared_ptr<TestCaseInformation>& rhs) const
  {
    return lhs->get_test_case_key() < rhs->get_test_case_key();
  }
};

extern bool compare_test_case_variants(const std::shared_ptr<TestCaseInformation>& lhs, const std::shared_ptr<TestCaseInformation>& rhs);

class TemporalTestCaseInformation: public TestCaseInformation
{
public:
  TemporalTestCaseInformation(
    const std::string& region_name,
    const std::string& temporal_bug_name,
    const std::string& temporal_memory_state_name,
    const std::string& access_location_name,
    const std::string& access_action_name,
    const std::string& file_name,
    const std::string& file_name_without_suffix,
    bool is_validation,
    int variant_number
  );

  std::string region_name;
  std::string temporal_bug_name;
  std::string temporal_memory_state_name;
  std::string access_location_name;
  std::string access_action_name;
};

class SpatialTestCaseInformation: public TestCaseInformation
{
public:
  SpatialTestCaseInformation(
    const std::string& origin_name,
    const std::string& target_name,
    const std::string& origin_target_relation_name,
    const std::string& spatial_bug_name,
    const std::string& flow_name,
    const std::string& access_location_name,
    const std::string& access_action_name,
    const std::string& file_name,
    const std::string& file_name_without_suffix,
    bool is_validation,
    int variant_number
  );

  std::string origin_name;
  std::string target_name;
  std::string origin_target_relation_name;
  std::string spatial_bug_name;
  std::string flow_name;
  std::string access_location_name;
  std::string access_action_name;
};


extern std::shared_ptr<TestCaseInformation> process_file_name(const std::string &file_name);
