
#ifndef VOXEL_ENGINE_TIME_COMPUTE_INC
#define VOXEL_ENGINE_TIME_COMPUTE_INC

#include "common.inc"

COMPUTE_SHADER_BUFFER(${common.time_uniform}, readonly, u_compute_time_buff_t, u_compute_time_buff, {
    float time;
})

float getTime() {
    return u_compute_time_buff.time;
}

#endif  // VOXEL_ENGINE_TIME_COMPUTE_INC