#include "chunk_provider.h"

#include "voxel/engine/world/chunk.h"
#include "voxel/engine/world/chunk_source.h"


namespace voxel {

bool ChunkProvider::canFetchChunk(ChunkSource& chunk_source, ChunkPosition position) {
    return false;
}

Shared<Chunk> ChunkProvider::createChunk(ChunkSource& chunk_source, ChunkPosition position) {
    return Shared<Chunk>();
}

bool ChunkProvider::buildChunk(ChunkSource& chunk_source, Shared<Chunk> chunk) {
    return true;
}

bool ChunkProvider::processChunk(ChunkSource& chunk_source, Shared<Chunk> chunk) {
    return true;
}

}