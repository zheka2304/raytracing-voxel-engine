#ifndef VOXEL_ENGINE_PREMADE_H
#define VOXEL_ENGINE_PREMADE_H

#include "voxel/common/base.h"
#include "voxel/engine/world.h"
#include "voxel/engine/shared/voxel_model.h"


namespace voxel {

Shared<VoxelModel> loadVoxelModelFromVoxFile(const std::string& filename);

Unique<World> createWorldFromModel(Shared<VoxelModel> model, Voxel ground_voxel);

} // voxel

#endif //VOXEL_ENGINE_PREMADE_H
