#include "chunk_provider.h"

#include "voxel/engine/world/chunk.h"
#include "voxel/engine/world/chunk_source.h"


namespace voxel {

bool ChunkProvider::canFetchChunk(ChunkSource& chunk_source, ChunkPosition position) {
    return false;
}

Unique<Chunk> ChunkProvider::createChunk(ChunkSource& chunk_source, ChunkPosition position) {
    return nullptr;
}

bool ChunkProvider::buildChunk(ChunkSource& chunk_source, Chunk& chunk) {
    return true;
}

bool ChunkProvider::processChunk(ChunkSource& chunk_source, Chunk& chunk) {
    return true;
}

}