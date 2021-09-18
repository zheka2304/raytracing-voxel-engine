#ifndef VOXEL_ENGINE_VOXEL_H
#define VOXEL_ENGINE_VOXEL_H

#include "voxel/common/types.h"
#include "voxel/common/math/color.h"
#include "voxel/common/math/vec.h"


namespace voxel {

struct VoxelMaterial {
    u8 blend_mode;
    bool ignore_lighting;
    math::Vec3f normal;

    u32 pack();
};

struct Voxel {
    u32 color = 0u;
    u32 material = 0u;

    static Voxel create(math::Color color);
    static Voxel create(math::Color color, VoxelMaterial mat);
};

} // voxel

#endif //VOXEL_ENGINE_VOXEL_H
