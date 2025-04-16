/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#pragma once

#include <memory>
#include <set>

#include "bug_types/spatial/spatial_bug_type.h"
#include "bug_types/temporal/temporal_bug_type.h"
#include "bug_types/temporal/memory_state/memory_state.h"
#include "generator/primitives/access_types/access_action.h"
#include "generator/primitives/access_types/access_location.h"
#include "generator/primitives/regions/region.h"

/**
 * Memory regions
 */
extern std::set< std::shared_ptr<Region> > memory_regions;

/**
 * Temporal memory bug types
 */
extern std::set< std::shared_ptr<MemoryState> > memory_states;
extern std::set< std::shared_ptr<TemporalBugType> > temporal_bug_types;

/**
 * Spatial memory bug types
 */
extern std::set< std::shared_ptr<SpatialBugType> > spatial_bug_types;
extern std::set< std::shared_ptr<Flow> > flows;
extern std::set< std::shared_ptr<OriginTargetRelation> > origin_target_relations;

/**
 * Access types
 */
extern std::set< std::shared_ptr<AccessAction> > access_type_actions;
extern std::set< std::shared_ptr<AccessLocation> > access_type_locations;
