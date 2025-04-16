/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "generator/generator.h"

#include <fstream>
#include <iostream>
#include <cstring>
#include <unistd.h>

#include "misc.h"
#include "generator/primitives/primitive_pool.h"

static void generate_file(const std::string& dir_path, const std::string& file_name, const std::shared_ptr<CodeCanvas>& code)
{
  std::string full_path = dir_path + file_name + ".c";
  std::ofstream file(full_path);

  if (!file)
  {
    std::cerr << "Error opening file: " << full_path << ": " << std::strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }

  file << code->to_string();
}

std::string build_file_name(
  const std::shared_ptr<TemporalBugType> & bug_type,
  const std::shared_ptr<MemoryState> & mem_state,
  const std::shared_ptr<Region> & region,
  const std::shared_ptr<AccessAction> & access_action,
  const std::shared_ptr<AccessLocation> & access_location
)
{
  return bug_type->get_name() + "_" + mem_state->get_name() + "_" + region->get_name() + "_" + access_location->get_name() + "_" + access_action->get_name();
}

std::string build_file_name(
  const std::shared_ptr<SpatialBugType> & bug_type,
  const std::shared_ptr<Region> & origin,
  const std::shared_ptr<Region> & target,
  const std::shared_ptr<OriginTargetRelation> & origin_target_relation,
  const std::shared_ptr<Flow> & flow,
  const std::shared_ptr<AccessAction> & access_action,
  const std::shared_ptr<AccessLocation> & access_location
)
{
  return bug_type->get_name() + "_" + origin->get_name() + "_" + target->get_name() + "_" + origin_target_relation->get_name() + "_" + flow->get_name()
    + "_" + access_location->get_name() + "_" + access_action->get_name();
}

void generate(const std::string& dir_path)
{
  size_t temporal_generated_counter = 0;
  for ( auto temporal_bug_type: temporal_bug_types )
  {
    for ( auto memory_state: memory_states )
    {
      if ( !temporal_bug_type->accepts(memory_state) )
      {
        continue;
      }

      for ( auto memory_region: memory_regions )
      {
        if ( !memory_state->accepts(memory_region) )
        {
          continue;
        }
        if ( !temporal_bug_type->accepts(memory_region) )
        {
          continue;
        }

        for ( auto access_action: access_type_actions )
        {
          for ( auto access_location: access_type_locations )
          {
            std::vector< std::shared_ptr<RegionCodeCanvas> > code_canvas_variants = temporal_bug_type->generate(memory_state, memory_region, access_action, access_location);
            std::string file_name = build_file_name(temporal_bug_type, memory_state, memory_region, access_action, access_location);
            size_t variant_index = 0;
            for ( const auto &code_canvas: code_canvas_variants )
            {
              temporal_generated_counter++;
              generate_file(dir_path, file_name + "_" + std::to_string(variant_index), code_canvas);
              variant_index++;
            }

            std::vector< std::shared_ptr<RegionCodeCanvas> > code_canvas_validation_variants = temporal_bug_type->generate_validation(memory_state, memory_region, access_action, access_location);
            file_name = build_file_name(temporal_bug_type, memory_state, memory_region, access_action, access_location);
            variant_index = 0;
            for ( const auto &code_canvas: code_canvas_validation_variants )
            {
              generate_file(dir_path, file_name + "_validation_" + std::to_string(variant_index), code_canvas);
              variant_index++;
            }
          }
        }
      }
    }
  }
  std::cout << "Generated " << temporal_generated_counter << " temporal variants\n";

  size_t spatial_generated_counter = 0;
  for ( auto spatial_bug_type: spatial_bug_types )
  {
    for ( auto flow: flows )
    {
      if ( !spatial_bug_type->accepts(flow) )
      {
        continue;
      }

      for ( auto origin: memory_regions )
      {
        for ( auto target: memory_regions )
        {
          for ( auto origin_target_relation: origin_target_relations )
          {
            if ( !origin_target_relation->accepts(origin, target) )
            {
              continue;
            }
            if ( !spatial_bug_type->accepts(origin_target_relation) )
            {
              continue;
            }

            for ( auto access_action: access_type_actions )
            {
              for ( auto access_location: access_type_locations )
              {
                if ( !flow->accepts(access_location) )
                {
                  continue;
                }
                if ( !spatial_bug_type->accepts(access_location) )
                {
                  continue;
                }
                std::vector< std::shared_ptr<OriginTargetCodeCanvas> > code_canvas_variants = spatial_bug_type->generate(
                  origin, target, origin_target_relation,
                  flow,
                  access_action, access_location
                );
                std::string file_name = build_file_name(
                  spatial_bug_type,
                  origin, target, origin_target_relation,
                  flow,
                  access_action, access_location
                );
                size_t variant_index = 0;
                for ( const auto &code_canvas: code_canvas_variants )
                {
                  spatial_generated_counter++;
                  generate_file(dir_path, file_name + "_" + std::to_string(variant_index), code_canvas);
                  variant_index++;
                }

                std::vector< std::shared_ptr<OriginTargetCodeCanvas> > code_canvas_validation_variants = spatial_bug_type->generate_validation(
                  origin, target, origin_target_relation,
                  flow,
                  access_action, access_location
                  );
                variant_index = 0;
                for ( const auto &code_canvas: code_canvas_validation_variants )
                {
                  generate_file(dir_path, file_name + "_validation_" + std::to_string(variant_index), code_canvas);
                  variant_index++;
                }
              }
            }
          }

        }
      }
    }
  }
  std::cout << "Generated " << spatial_generated_counter << " spatial variants\n";
  std::cout << "Generated " << spatial_generated_counter + temporal_generated_counter << " variants\n";
}
