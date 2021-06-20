#ifndef VOXEL_ENGINE_CAMERA_INC
#define VOXEL_ENGINE_CAMERA_INC

#include "common.inc"
#include "raytrace/types.inc"


COMPUTE_SHADER_BUFFER(${raytrace.camera_projection}, readonly, u_camera_projection_t, u_camera_projection, {
    vec3 position;
    vec3 rotation;
    vec2 ortho_range;
    vec2 perspective;
    vec2 distance_range;
})

#endif  // VOXEL_ENGINE_CAMERA_INC