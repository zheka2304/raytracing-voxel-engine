#include "chunk_storage.h"

#include "voxel/engine/world/chunk.h"
#include "voxel/engine/world/chunk_source.h"


namespace voxel {

bool ChunkStorage::tryStoreChunk(ChunkSource& chunk_source, Chunk&) {
    return true;
}

bool ChunkStorage::tryLoadChunk(ChunkSource& chunk_source, Chunk&) {
    return false;
}

}