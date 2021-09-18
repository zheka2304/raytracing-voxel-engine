#ifndef VOXEL_ENGINE_FILE_FORMAT_H
#define VOXEL_ENGINE_FILE_FORMAT_H

#include <memory>
#include <istream>
#include <vector>
#include "voxel/engine/shared/voxel_model.h"


namespace voxel {
namespace format {

class FileFormat {
public:
    virtual std::vector<std::unique_ptr<VoxelModel>> read(std::istream& istream) = 0;
};

} // format
} // voxel

#endif //VOXEL_ENGINE_FILE_FORMAT_H
