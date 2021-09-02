#ifndef VOXEL_ENGINE_RAYTRACE_TYPES_INC
#define VOXEL_ENGINE_RAYTRACE_TYPES_INC

struct Ray {
    vec3 start;
    vec3 ray;
    float max_distance;
};

struct RaycastResult {
    uint voxel_index;
    uint voxel;

    float distance;
    vec3 end;
    vec3 normal;
    vec3 color;
    uint steps;
};

#endif  // VOXEL_ENGINE_RAYTRACE_TYPES_INC