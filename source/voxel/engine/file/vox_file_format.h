#ifndef VOXEL_ENGINE_VOX_FILE_FORMAT_H
#define VOXEL_ENGINE_VOX_FILE_FORMAT_H

#include "voxel/engine/file/riff_file_format.h"

namespace voxel {
namespace format {

class VoxFileFormat : public RiffStyleFileFormat {
public:
    std::vector<VoxelModel> readRiff(RiffFile& riff_file) override;
};

} // format
} // voxel

#endif //VOXEL_ENGINE_VOX_FILE_FORMAT_H
