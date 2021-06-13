#ifndef VOXEL_ENGINE_VOXEL_PALETTE_H
#define VOXEL_ENGINE_VOXEL_PALETTE_H

#include "common/types.h"
#include "common/color.h"

class VoxelType {
public:
    voxel_id_t id;
    Color color;
    int lightIntensity;

    VoxelType(voxel_id_t voxel_id);
};

#endif //VOXEL_ENGINE_VOXEL_PALETTE_H
