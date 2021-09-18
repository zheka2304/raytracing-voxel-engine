#ifndef VOXEL_ENGINE_VOXEL_MODEL_H
#define VOXEL_ENGINE_VOXEL_MODEL_H

#include <vector>

#include "voxel/common/math/vec.h"
#include "voxel/engine/shared/voxel.h"


namespace voxel {

class VoxelModel {
    math::Vec3i m_size;
    std::vector<Voxel> m_voxels;
public:
    VoxelModel(i32 size_x, i32 size_y, i32 size_z);
    math::Vec3i getSize();
    std::vector<Voxel>& getVoxels();

    Voxel getVoxel(i32 x, i32 y, i32 z);
    void setVoxel(i32 x, i32 y, i32 z, Voxel voxel);
};

} // voxel

#endif //VOXEL_ENGINE_VOXEL_MODEL_H
