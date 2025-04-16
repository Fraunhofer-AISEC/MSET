/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "generator/primitives/primitive_pool.h"

#include "generator/primitives/access_types/direct_location.h"
#include "generator/primitives/access_types/read_action.h"
#include "generator/primitives/access_types/stdlib_location.h"
#include "generator/primitives/access_types/write_action.h"
#include "generator/primitives/bug_types/spatial/linear_ooba.h"
#include "generator/primitives/bug_types/spatial/non_linear_ooba.h"
#include "generator/primitives/bug_types/spatial/type_confusion.h"
#include "generator/primitives/bug_types/spatial/flow/overflow.h"
#include "generator/primitives/bug_types/spatial/flow/underflow.h"
#include "generator/primitives/bug_types/spatial/origin_target_relation/inter_object.h"
#include "generator/primitives/bug_types/spatial/origin_target_relation/intra_object.h"
#include "generator/primitives/bug_types/spatial/origin_target_relation/non_object.h"
#include "generator/primitives/bug_types/temporal/double_free.h"
#include "generator/primitives/bug_types/temporal/misuse_of_free.h"
#include "generator/primitives/bug_types/temporal/use_after_star.h"
#include "generator/primitives/bug_types/temporal/memory_state/freed_memory.h"
#include "generator/primitives/bug_types/temporal/memory_state/used.h"
#include "generator/primitives/regions/global_region.h"
#include "generator/primitives/regions/heap_region.h"
#include "generator/primitives/regions/stack_region.h"

/**
 * Memory regions
 */
std::set< std::shared_ptr<Region> > memory_regions = {
  std::make_shared<StackRegion>(),
  std::make_shared<HeapRegion>(),
  std::make_shared<GlobalRegion>()
};

/**
 * Temporal memory bug types
 */
std::set< std::shared_ptr<MemoryState> > memory_states = {
  std::make_shared<UsedMemory>(),
  std::make_shared<FreedMemory>()
};

std::set< std::shared_ptr<TemporalBugType> > temporal_bug_types = {
   std::make_shared<DoubleFree>(),
   std::make_shared<MisuseOfFree>(),
   std::make_shared<UseAfterStar>()
};


/**
 * Spatial memory bug types
 */
std::set< std::shared_ptr<Flow> > flows = {
  std::make_shared<Overflow>(),
  std::make_shared<Underflow>()
};

std::set< std::shared_ptr<OriginTargetRelation> > origin_target_relations = {
  std::make_shared<InterObject>(),
  std::make_shared<IntraObject>(),
  std::make_shared<NonObject>()
};

std::set< std::shared_ptr<SpatialBugType> > spatial_bug_types = {
  std::make_shared<LinearOOBA>(),
  std::make_shared<NonLinearOOBA>(),
  std::make_shared<TypeConfusion>()
};

/**
 * Access types
 */
std::set< std::shared_ptr<AccessAction> > access_type_actions = {
  std::make_shared<ReadAction>(),
  std::make_shared<WriteAction>()
};

std::set< std::shared_ptr<AccessLocation> > access_type_locations = {
  std::make_shared<DirectLocation>(),
  std::make_shared<StdlibLocation>()
};
