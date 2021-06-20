#ifndef VOXEL_ENGINE_RAYTRACE_TYPES_INC
#define VOXEL_ENGINE_RAYTRACE_TYPES_INC

struct Ray {
    vec3 start;
    vec3 ray;
    float max_distance;
};

#endif  // VOXEL_ENGINE_RAYTRACE_TYPES_INC