
#ifndef VOXEL_ENGINE_CHUNK_SOURCE_H
#define VOXEL_ENGINE_CHUNK_SOURCE_H

#include <unordered_map>
#include <functional>

#include "common/chunk_pos.h"
#include "voxel_chunk.h"


class ChunkSource {
public:
    virtual VoxelChunk* getChunkAt(ChunkPos pos) = 0;
};


class DebugChunkSource : public ChunkSource {
public:
    std::unordered_map<ChunkPos, VoxelChunk*> chunkMap;
    std::function<VoxelChunk*(ChunkPos)> chunkProviderFunc;

    explicit DebugChunkSource(std::function<VoxelChunk*(ChunkPos)> providerFunc);
    VoxelChunk* getChunkAt(ChunkPos pos) override;
    ~DebugChunkSource();
};

#endif //VOXEL_ENGINE_CHUNK_SOURCE_H
