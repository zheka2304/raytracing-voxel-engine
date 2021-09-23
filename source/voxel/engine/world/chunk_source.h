#ifndef VOXEL_ENGINE_CHUNK_SOURCE_H
#define VOXEL_ENGINE_CHUNK_SOURCE_H

#include <unordered_map>
#include <mutex>

#include "voxel/common/base.h"
#include "voxel/common/threading.h"
#include "voxel/engine/world/chunk.h"
#include "voxel/engine/world/chunk_lock.h"


namespace voxel {
namespace world {

class ChunkStorage;
class ChunkProvider;


class ChunkSource {
    Shared<ChunkProvider> m_provider;
    Shared<ChunkStorage> m_storage;
    Shared<threading::TaskExecutor> m_executor;

    std::unordered_map<ChunkPosition, Shared<Chunk>> m_chunks;
    std::mutex m_chunks_mutex;

public:
    ChunkSource(Shared<ChunkProvider> provider, Shared<ChunkStorage> storage, Shared<threading::TaskExecutor> executor);
    ~ChunkSource();

    void onTick();
    Shared<Chunk> getChunkAt(ChunkPosition position);
    void fetchChunkAt(ChunkPosition position);

private:
    void handleChunk(Shared<Chunk> chunk);
    void handleChunkStateTask(Shared<Chunk> chunk);
    void handleLoadedChunk(Shared<Chunk> chunk);
    void handleLazyChunk(Shared<Chunk> chunk);

    void runChunkBuild(Shared<Chunk> chunk);
    void runChunkProcessing(Shared<Chunk> chunk);
    void runChunkLoad(Shared<Chunk> chunk);
    void runChunkUnload(Shared<Chunk> chunk);
};

} // world
} // voxel

#endif //VOXEL_ENGINE_CHUNK_SOURCE_H

