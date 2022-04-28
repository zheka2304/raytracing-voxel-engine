#ifndef VOXEL_ENGINE_CONFIG_H
#define VOXEL_ENGINE_CONFIG_H

#include <cassert>

// enable profiling
#define VOXEL_ENGINE_ENABLE_PROFILER 1

// enable additional logging
#define VOXEL_ENGINE_ENABLE_DEBUG_VERBOSE 1

// enable thread guards
#define VOXEL_ENGINE_ENABLE_THREAD_GUARD 1

// enables assert checks in the engine
#define VOXEL_ENGINE_ENABLE_ASSERT 1

#if VOXEL_ENGINE_ENABLE_ASSERT
#define VOXEL_ENGINE_ASSERT(...) assert(__VA_ARGS__)
#else
#define VOXEL_ENGINE_ASSERT(...)
#endif

#endif //VOXEL_ENGINE_CONFIG_H
