
#ifndef VOXEL_ENGINE_SPATIAL_BUFFER_INC
#define VOXEL_ENGINE_SPATIAL_BUFFER_INC

#include "common.inc"


struct SpatialBufferData {
    // x, y, depth, priority
    ivec4 pixel;

    // pixel error x, y, reserved for last stability, stability
    vec4 value;
};

COMPUTE_SHADER_BUFFER(${common.spatial_buffer}, , spatial_buffer_t, spatial_buffer, {
    SpatialBufferData data[];
})

COMPUTE_SHADER_BUFFER(${common.spatial_buffer_spinlock}, , spatial_buffer_spinlock_t, spatial_buffer_spinlock, {
    uint data[];
})


int getSpatialBufferIndex(ivec2 pixel_pos) {
    return U_SCREEN_SIZE.x * pixel_pos.y + pixel_pos.x;
}

#define SPATIAL_BUFFER spatial_buffer.data
#define ACCESS_SPATIAL_BUFFER(X) spatial_buffer.data[getSpatialBufferIndex(X)]


#endif  // VOXEL_ENGINE_SPATIAL_BUFFER_INC