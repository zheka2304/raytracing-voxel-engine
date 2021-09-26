#ifndef VOXEL_ENGINE_CHUNK_PROVIDER_H
#define VOXEL_ENGINE_CHUNK_PROVIDER_H

#include "voxel/common/base.h"
#include "voxel/engine/shared/chunk_position.h"


namespace voxel {

class Chunk;
class ChunkSource;

class ChunkProvider {
public:
    // Quick check, if chunk at position can be created,
    virtual bool canFetchChunk(ChunkSource& chunk_source, ChunkPosition position);

    // Create new empty chunk for position, can perform more heavy check, than canFetchChunk and return empty pointer
    virtual Shared<Chunk> createChunk(ChunkSource& chunk_source, ChunkPosition position);

    // Handle chunk build task, return true, if succeeded
    virtual bool buildChunk(ChunkSource& chunk_source, Shared<Chunk> chunk);

    // Handle chunk processing task, return true, if succeeded
    virtual bool processChunk(ChunkSource& chunk_source, Shared<Chunk> chunk);
};

}

#endif //VOXEL_ENGINE_CHUNK_PROVIDER_H
