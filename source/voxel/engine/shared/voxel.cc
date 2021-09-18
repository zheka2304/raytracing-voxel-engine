#include "voxel.h"


namespace voxel {

u32 VoxelMaterial::pack() {
    u32 packed = 0u;
    packed |= (blend_mode & 63u);
    return packed;
}

Voxel Voxel::create(math::Color color) {
    return { color.toHex(8, 5), 0 };
}

Voxel Voxel::create(math::Color color, VoxelMaterial mat) {
    return { color.toHex(8, 5), mat.pack() };
}

} // voxel