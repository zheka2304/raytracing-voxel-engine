#include "voxel_model.h"


namespace voxel {

VoxelModel::VoxelModel(i32 size_x, i32 size_y, i32 size_z) :
    m_size(size_x, size_y, size_z),
    m_voxels(size_x * size_y * size_z) {
}

math::Vec3i VoxelModel::getSize() {
    return m_size;
}

std::vector<Voxel>& VoxelModel::getVoxels() {
    return m_voxels;
}

Voxel VoxelModel::getVoxel(i32 x, i32 y, i32 z) {
    return m_voxels[x + (y + z * m_size.y) * m_size.x];
}

void VoxelModel::setVoxel(i32 x, i32 y, i32 z, Voxel voxel) {
    m_voxels[x + (y + z * m_size.y) * m_size.x] = voxel;
}

} // voxel