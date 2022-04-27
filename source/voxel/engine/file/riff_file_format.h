#ifndef VOXEL_ENGINE_RIFF_FILE_FORMAT_H
#define VOXEL_ENGINE_RIFF_FILE_FORMAT_H

#include "voxel/engine/file/file_format.h"


namespace voxel {
namespace format {

class RiffStyleFileFormat : public FileFormat {
public:
    struct RiffChunk {
        char id[5];
        std::vector<byte> bytes;
        std::vector<RiffChunk> children;
        u32 total_size;

        template<typename T>
        T* asSpan() {
            return reinterpret_cast<T*>(&bytes[0]);
        }

        template<typename T>
        T& asStruct() {
            if (bytes.size() < sizeof(T)) {
                throw std::bad_cast();
            }
            return *reinterpret_cast<T*>(&bytes[0]);
        }
    };

    struct RiffFile {
        char type[5];
        u32 version;
        std::vector<RiffChunk> chunks;
    };

public:
    std::vector<VoxelModel> read(std::istream& istream) override;
    virtual std::vector<VoxelModel> readRiff(RiffFile& riff_file);

private:
    RiffChunk _readRiffChunkRecursive(std::istream& istream, i32 depth = 0);
};

} // format
} // voxel

#endif //VOXEL_ENGINE_RIFF_FILE_FORMAT_H
