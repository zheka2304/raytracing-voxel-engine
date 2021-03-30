#include "chunk_source.h"


DebugChunkSource::DebugChunkSource(std::function<VoxelChunk *(ChunkPos)> providerFunc) : chunkProviderFunc(std::move(providerFunc)) {

}

VoxelChunk* DebugChunkSource::getChunkAt(ChunkPos pos) {
    auto it = chunkMap.find(pos);
    if (it != chunkMap.end()) {
        return it->second;
    }
    VoxelChunk* chunk = chunkProviderFunc(pos);
    if (chunk != nullptr) {
        chunkMap.emplace(pos, chunk);
    }
    return chunk;
}

DebugChunkSource::~DebugChunkSource() {
    for (auto posAndChunk : chunkMap) {
        delete(posAndChunk.second);
    }
}

