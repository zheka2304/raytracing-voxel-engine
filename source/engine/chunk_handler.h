
#ifndef VOXEL_ENGINE_CHUNK_HANDLER_H
#define VOXEL_ENGINE_CHUNK_HANDLER_H

#include "common/vec.h"
#include "voxel_chunk.h"

// interface, implementing logic of requesting and unloading chunks
// used by chunk sources
class ChunkHandler {
public:
    // check if position valid for request
    // should not contain heavy logic
    virtual bool isValidPosition(ChunkPos const& pos);

    // loads data for given chunk
    // this will run in background, so may contain heavy logic
    // returns, if chunk data was loaded successfully
    virtual bool requestChunk(VoxelChunk& chunk);
    virtual Vec2i getRequestLockRadius();

    // runs baking operation for given chunk
    // this will run in background, so may contain heavy logic
    virtual void bakeChunk(VoxelChunk& chunk);
    virtual Vec2i getBakeLockRadius();

    // unloads given chunk, after this it will be destroyed
    virtual void unloadChunk(VoxelChunk& chunk);
};

#endif //VOXEL_ENGINE_CHUNK_HANDLER_H
