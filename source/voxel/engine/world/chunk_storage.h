#ifndef VOXEL_ENGINE_CHUNK_STORAGE_H
#define VOXEL_ENGINE_CHUNK_STORAGE_H

#include "voxel/common/base.h"
#include "voxel/engine/shared/chunk_position.h"


namespace voxel {

class Chunk;
class ChunkSource;

class ChunkStorage {
public:
    // attempts to store chunk data in storage, returns true, if chunk can process to unloading
    virtual bool tryStoreChunk(ChunkSource& chunk_source, Chunk&);

    // attempts to load chunk from storage, returns true, if chunk was loaded and should skip build and processing stages
    virtual bool tryLoadChunk(ChunkSource& chunk_source, Chunk&);
};

} // voxel

#endif //VOXEL_ENGINE_CHUNK_STORAGE_H
