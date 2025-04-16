/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "test_case_information.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <ostream>
#include <utility>

static ssize_t find_prefix( std::string &in, const std::vector<std::string> &prefixes )
{
  size_t i = 0;
  for ( const auto& prefix: prefixes )
  {
    if ( in.find(prefix) == 0 )
    {
      in = in.substr(prefix.length());
      return i;
    }
    i++;
  }
  return -1;
}

std::shared_ptr<TestCaseInformation> TestCaseInformation::construct_from_file_name(const std::string &file_name)
{
  std::string remaining_string = file_name;
  bool is_validation = false;

  // e.g., misuse_of_free_freed_memory_global_direct_read_0
  ssize_t found_at = find_prefix(remaining_string, temporal_bug_types);
  bool is_temporal = found_at != -1;
  if ( is_temporal )
  {
    std::string temporal_bug = temporal_bugs_info[found_at];

    if ( remaining_string.empty() )
    {
      std::cerr << "Unsupported file name. Expected temporal memory state." << std::endl;
      exit(EXIT_FAILURE);
    }
    if ( remaining_string[0] != '_' )
    {
      std::cerr << "Unsupported file name. Expected '_' before " << remaining_string << std::endl;
      exit(EXIT_FAILURE);
    }
    remaining_string = remaining_string.substr(1); // consume the _
    found_at = find_prefix(remaining_string, temporal_memory_states);
    if (found_at == -1)
    {
      std::cerr << "Unsupported file name. Expected temporal memory state before " << remaining_string << std::endl;
      exit(EXIT_FAILURE);
    }
    std::string temporal_memory_state = temporal_memory_states_info[found_at];

    if ( remaining_string.empty() )
    {
      std::cerr << "Unsupported file name. Expected region." << std::endl;
      exit(EXIT_FAILURE);
    }
    if ( remaining_string[0] != '_' )
    {
      std::cerr << "Unsupported file name. Expected '_' before " << remaining_string << std::endl;
      exit(EXIT_FAILURE);
    }
    remaining_string = remaining_string.substr(1); // consume the _
    found_at = find_prefix(remaining_string, regions);
    if (found_at == -1)
    {
      std::cerr << "Unsupported file name. Expected region before " << remaining_string << std::endl;
      exit(EXIT_FAILURE);
    }
    std::string region = regions_info[found_at];

    if ( remaining_string.empty() )
    {
      std::cerr << "Unsupported file name. Expected access location." << std::endl;
      exit(EXIT_FAILURE);
    }
    if ( remaining_string[0] != '_' )
    {
      std::cerr << "Unsupported file name. Expected '_' before " << remaining_string << std::endl;
      exit(EXIT_FAILURE);
    }
    remaining_string = remaining_string.substr(1); // consume the _
    found_at = find_prefix(remaining_string, access_locations);
    if (found_at == -1)
    {
      std::cerr << "Unsupported file name. Expected access location before " << remaining_string << std::endl;
      exit(EXIT_FAILURE);
    }
    std::string access_location = access_locations_info[found_at];

    if ( remaining_string.empty() )
    {
      std::cerr << "Unsupported file name. Expected access action." << std::endl;
      exit(EXIT_FAILURE);
    }
    if ( remaining_string[0] != '_' )
    {
      std::cerr << "Unsupported file name. Expected '_' before " << remaining_string << std::endl;
      exit(EXIT_FAILURE);
    }
    remaining_string = remaining_string.substr(1); // consume the _
    found_at = find_prefix(remaining_string, access_actions);
    if (found_at == -1)
    {
      std::cerr << "Unsupported file name. Expected an access action before " << remaining_string << std::endl;
      exit(EXIT_FAILURE);
    }
    std::string access_action = access_actions_info[found_at];

    if ( remaining_string.empty() )
    {
      std::cerr << "Unsupported file name. Expected the variant number." << std::endl;
      exit(EXIT_FAILURE);
    }
    if ( remaining_string[0] != '_' )
    {
      std::cerr << "Unsupported file name. Expected '_' before " << remaining_string << std::endl;
      exit(EXIT_FAILURE);
    }
    remaining_string = remaining_string.substr(1); // consume the _
    if ( !std::isdigit(remaining_string[0]) )
    {
      if ( remaining_string.find("validation") == 0 )
      {
        is_validation = true;
        remaining_string = remaining_string.substr(strlen("validation"));
        if ( remaining_string.empty() )
        {
          std::cerr << "Unsupported file name. Expected the variant number." << std::endl;
          exit(EXIT_FAILURE);
        }
        if ( remaining_string[0] != '_' )
        {
          std::cerr << "Unsupported file name. Expected '_' before " << remaining_string << std::endl;
          exit(EXIT_FAILURE);
        }
        remaining_string = remaining_string.substr(1); // consume the _
      }
      else
      {
        std::cerr << "Unsupported file name. Expected a variant number or \"validation\" before " << remaining_string << std::endl;
        exit(EXIT_FAILURE);
      }
    }

    size_t index;
    if ( !std::isdigit(remaining_string[0]) )
    {
      std::cerr << "Unsupported file name. Expected a variant number before " << remaining_string << std::endl;
      exit(EXIT_FAILURE);
    }
    int variant_number = std::stoi(remaining_string, &index);
    remaining_string = remaining_string.substr(index);
    if ( remaining_string != ".c" )
    {
      std::cerr << "Unsupported file name. Must end with \".c\". Got " << remaining_string << std::endl;
      exit(EXIT_FAILURE);
    }

    assert(file_name.length() >= 2);
    std::string file_name_without_suffix = file_name.substr(0, file_name.length() - 2); // remove .c

    return std::make_shared<TemporalTestCaseInformation>(
      region,
      temporal_bug,
      temporal_memory_state,
      access_location,
      access_action,
      file_name,
      file_name_without_suffix,
      is_validation,
      variant_number
    );
  }

  found_at = find_prefix(remaining_string, spatial_bug_types);
  bool is_spatial = found_at != -1;
  if (!is_spatial) return {};

  std::string spatial_bug = spatial_bugs_info[found_at];

  if ( remaining_string.empty() )
  {
    std::cerr << "Unsupported file name. Expected spatial origin." << std::endl;
    exit(EXIT_FAILURE);
  }
  if ( remaining_string[0] != '_' )
  {
    std::cerr << "Unsupported file name. Expected '_' before " << remaining_string << std::endl;
    exit(EXIT_FAILURE);
  }
  remaining_string = remaining_string.substr(1); // consume the _
  found_at = find_prefix(remaining_string, regions);
  if (found_at == -1)
  {
    std::cerr << "Unsupported file name. Expected spatial origin memory state before " << remaining_string << std::endl;
    exit(EXIT_FAILURE);
  }
  std::string origin = regions_info[found_at];


  if ( remaining_string.empty() )
  {
    std::cerr << "Unsupported file name. Expected spatial target." << std::endl;
    exit(EXIT_FAILURE);
  }
  if ( remaining_string[0] != '_' )
  {
    std::cerr << "Unsupported file name. Expected '_' before " << remaining_string << std::endl;
    exit(EXIT_FAILURE);
  }
  remaining_string = remaining_string.substr(1); // consume the _
  found_at = find_prefix(remaining_string, regions);
  if (found_at == -1)
  {
    std::cerr << "Unsupported file name. Expected spatial target memory " << remaining_string << std::endl;
    exit(EXIT_FAILURE);
  }
  std::string target = regions_info[found_at];


  if ( remaining_string.empty() )
  {
    std::cerr << "Unsupported file name. Expected origin-target relation." << std::endl;
    exit(EXIT_FAILURE);
  }
  if ( remaining_string[0] != '_' )
  {
    std::cerr << "Unsupported file name. Expected '_' before " << remaining_string << std::endl;
    exit(EXIT_FAILURE);
  }
  remaining_string = remaining_string.substr(1); // consume the _
  found_at = find_prefix(remaining_string, origin_target_relations);
  if (found_at == -1)
  {
    std::cerr << "Unsupported file name. Expected origin-target relation before " << remaining_string << std::endl;
    exit(EXIT_FAILURE);
  }
  std::string origin_target_relation = origin_target_relations_info[found_at];


  if ( remaining_string.empty() )
  {
    std::cerr << "Unsupported file name. Expected flow name." << std::endl;
    exit(EXIT_FAILURE);
  }
  if ( remaining_string[0] != '_' )
  {
    std::cerr << "Unsupported file name. Expected '_' before " << remaining_string << std::endl;
    exit(EXIT_FAILURE);
  }
  remaining_string = remaining_string.substr(1); // consume the _
  found_at = find_prefix(remaining_string, flows);
  if (found_at == -1)
  {
    std::cerr << "Unsupported file name. Expected flow name before " << remaining_string << std::endl;
    exit(EXIT_FAILURE);
  }
  std::string flow = flows_info[found_at];

  if ( remaining_string.empty() )
  {
    std::cerr << "Unsupported file name. Expected access location." << std::endl;
    exit(EXIT_FAILURE);
  }
  if ( remaining_string[0] != '_' )
  {
    std::cerr << "Unsupported file name. Expected '_' before " << remaining_string << std::endl;
    exit(EXIT_FAILURE);
  }
  remaining_string = remaining_string.substr(1); // consume the _
  found_at = find_prefix(remaining_string, access_locations);
  if (found_at == -1)
  {
    std::cerr << "Unsupported file name. Expected access location before " << remaining_string << std::endl;
    exit(EXIT_FAILURE);
  }
  std::string access_location = access_locations_info[found_at];

  if ( remaining_string.empty() )
  {
    std::cerr << "Unsupported file name. Expected access action." << std::endl;
    exit(EXIT_FAILURE);
  }
  if ( remaining_string[0] != '_' )
  {
    std::cerr << "Unsupported file name. Expected '_' before " << remaining_string << std::endl;
    exit(EXIT_FAILURE);
  }
  remaining_string = remaining_string.substr(1); // consume the _
  found_at = find_prefix(remaining_string, access_actions);
  if (found_at == -1)
  {
    std::cerr << "Unsupported file name. Expected an access action before " << remaining_string << std::endl;
    exit(EXIT_FAILURE);
  }
  std::string access_action = access_actions_info[found_at];

  if ( remaining_string.empty() )
  {
    std::cerr << "Unsupported file name. Expected the variant number." << std::endl;
    exit(EXIT_FAILURE);
  }
  if ( remaining_string[0] != '_' )
  {
    std::cerr << "Unsupported file name. Expected '_' before " << remaining_string << std::endl;
    exit(EXIT_FAILURE);
  }
  remaining_string = remaining_string.substr(1); // consume the _
  if ( !std::isdigit(remaining_string[0]) )
  {
    if ( remaining_string.find("validation") == 0 )
    {
      is_validation = true;
      remaining_string = remaining_string.substr(strlen("validation"));
      if ( remaining_string.empty() )
      {
        std::cerr << "Unsupported file name. Expected the variant number." << std::endl;
        exit(EXIT_FAILURE);
      }
      if ( remaining_string[0] != '_' )
      {
        std::cerr << "Unsupported file name. Expected '_' before " << remaining_string << std::endl;
        exit(EXIT_FAILURE);
      }
      remaining_string = remaining_string.substr(1); // consume the _
    }
    else
    {
      std::cerr << "Unsupported file name. Expected a variant number or \"validation\" before " << remaining_string << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  if ( !std::isdigit(remaining_string[0]) )
  {
    std::cerr << "Unsupported file name. Expected a variant number before " << remaining_string << std::endl;
    exit(EXIT_FAILURE);
  }
  size_t index;
  int variant_number = std::stoi(remaining_string, &index);

  remaining_string = remaining_string.substr(index);
  if ( remaining_string != ".c" )
  {
    std::cerr << "Unsupported file name. Must end with \".c\". Got " << remaining_string << std::endl;
    exit(EXIT_FAILURE);
  }

  assert(file_name.length() >= 2);
  std::string file_name_without_suffix = file_name.substr(0, file_name.length() - 2); // remove .c

  return std::make_shared<SpatialTestCaseInformation>(
    origin,
    target,
    origin_target_relation,
    spatial_bug,
    flow,
    access_location,
    access_action,
    file_name,
    file_name_without_suffix,
    is_validation,
    variant_number
  );
}


TestCaseInformation::TestCaseInformation(bool is_validation, int variant_number, const std::string &file_name, const std::string &file_name_without_suffix):
  file_name(file_name),
  file_name_without_suffix(file_name_without_suffix),
  is_validation(is_validation),
  variant_number(variant_number)
{

}


TemporalTestCaseInformation::TemporalTestCaseInformation(
  const std::string& region_name,
  const std::string& temporal_bug_name,
  const std::string& temporal_memory_state_name,
  const std::string& access_location_name,
  const std::string& access_action_name,
  const std::string& file_name,
  const std::string& file_name_without_suffix,
  const bool is_validation,
  const int variant_number
):
  TestCaseInformation(is_validation, variant_number, file_name, file_name_without_suffix),
  region_name(region_name),
  temporal_bug_name(temporal_bug_name),
  temporal_memory_state_name(temporal_memory_state_name),
  access_location_name(access_location_name),
  access_action_name(access_action_name)
{
  as_string = temporal_bug_name + " " + access_location_name + " " + access_action_name + " on " +
      region_name + " " + temporal_memory_state_name + ", variant " + std::to_string(variant_number);
  key = temporal_bug_name + " " + access_location_name + " " + access_action_name + " " +
      region_name + " " + temporal_memory_state_name;
}


SpatialTestCaseInformation::SpatialTestCaseInformation(
  const std::string& origin_name,
  const std::string& target_name,
  const std::string& origin_target_relation_name,
  const std::string& spatial_bug_name,
  const std::string& flow_name,
  const std::string& access_location_name,
  const std::string& access_action_name,
  const std::string &file_name,
  const std::string &file_name_without_suffix,
  const bool is_validation,
  const int variant_number
  ):
  TestCaseInformation(is_validation, variant_number, file_name, file_name_without_suffix),
  origin_name(origin_name),
  target_name(target_name),
  origin_target_relation_name(origin_target_relation_name),
  spatial_bug_name(spatial_bug_name),
  flow_name(flow_name),
  access_location_name(access_location_name),
  access_action_name(access_action_name)
{

  as_string = origin_target_relation_name + " " + spatial_bug_name + " " + flow_name + " " +
      access_location_name + " " + access_action_name + " on (" +
      origin_name + ", " + target_name + "), variant " + std::to_string(variant_number);
  key = origin_target_relation_name + " " + spatial_bug_name + " " + flow_name + " " +
      access_location_name + " " + access_action_name + " " +
      origin_name + " " + target_name;
}

bool compare_test_case_variants(const std::shared_ptr<TestCaseInformation>& lhs, const std::shared_ptr<TestCaseInformation>& rhs)
{
  if (lhs->get_is_validation() != rhs->get_is_validation())
  {
    return lhs->get_is_validation() > rhs->get_is_validation(); // validation variants first
  }
  if (lhs->get_variant_number() != rhs->get_variant_number())
  {
    return lhs->get_variant_number() < rhs->get_variant_number(); // variant number, smallest first
  }
  return lhs->get_test_case_key() < rhs->get_test_case_key();
}