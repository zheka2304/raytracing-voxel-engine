#ifndef VOXEL_ENGINE_VOXEL_POSITION_H
#define VOXEL_ENGINE_VOXEL_POSITION_H

#include "voxel/common/base.h"


namespace voxel {

struct VoxelPosition {
    u8 scale;
    u32 x, y, z;
};

} // voxel

#endif //VOXEL_ENGINE_VOXEL_POSITION_H
