#ifndef VOXEL_ENGINE_RAYTRACE_DEBUG_INC
#define VOXEL_ENGINE_RAYTRACE_DEBUG_INC

#include "raytrace/types.inc"


#define RAYTRACE_DEBUG_CHECKERBOARD_SIZE 1.0

vec3 raycastDebugCheckerboard(Ray ray) {
    // normalize
    ray.ray = normalize(ray.ray);

    // sky
    if (ray.ray.y * ray.start.y > 0.0) {
        return vec3(0.5, 0.5, 1.0);
    } else {
        vec3 floor_pos = ray.start - ray.ray * (ray.start.y / ray.ray.y);
        ivec2 checkerboard_pos = ivec2(floor(floor_pos.xz / RAYTRACE_DEBUG_CHECKERBOARD_SIZE));
        return ((checkerboard_pos.x + checkerboard_pos.y) & 1) == 0 ? vec3(1.0) : vec3(0.0);
    }
}

#endif  // VOXEL_ENGINE_RAYTRACE_DEBUG_INC