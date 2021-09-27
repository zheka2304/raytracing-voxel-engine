#ifndef VOXEL_ENGINE_CHUNK_SOURCE_H
#define VOXEL_ENGINE_CHUNK_SOURCE_H

#include <unordered_map>
#include <mutex>

#include "voxel/common/base.h"
#include "voxel/common/threading.h"
#include "voxel/engine/world/chunk.h"
#include "voxel/engine/world/chunk_lock.h"


namespace voxel {

class ChunkStorage;
class ChunkProvider;
class ChunkSource;

class ChunkSourceListener {
public:
    virtual void onChunkSourceTick(ChunkSource& chunk_source);
    virtual void onChunkUpdated(ChunkSource& chunk_source, Shared<Chunk> chunk);
};

class ChunkSource {
public:
    enum ChunkSourceState {
        // normal state, load all chunks in background
        STATE_LOADED,

        // unloaded state, mark all existing chunks for unloading as soon as possible and do not load chunks
        STATE_UNLOADED,

        // lazy state, mark all loaded chunks lazy and start unloading rarely fetched chunks
        STATE_LAZY
    };

private:
    Shared<ChunkProvider> m_provider;
    Shared<ChunkStorage> m_storage;
    Shared<threading::TaskExecutor> m_executor;

    ChunkSourceState m_state;
    std::vector<ChunkSourceListener*> m_listeners;

    std::unordered_map<ChunkPosition, Shared<Chunk>> m_chunks;
    threading::BlockingQueue<Weak<Chunk>> m_updates_queue;
    std::mutex m_chunks_mutex;

public:
    ChunkSource(Shared<ChunkProvider> provider,
                Shared<ChunkStorage> storage,
                Shared<threading::TaskExecutor> executor,
                ChunkSourceState initial_state = STATE_LOADED);
    ChunkSource(const ChunkSource&) = delete;
    ChunkSource(ChunkSource&&) = delete;
    ~ChunkSource();

    ChunkSourceState getState();
    void setState(ChunkSourceState state);
    void addListener(ChunkSourceListener* listener);
    void removeListener(ChunkSourceListener* listener);

    void onTick();
    Shared<Chunk> getChunkAt(ChunkPosition position);
    bool fetchChunkAt(ChunkPosition position);

private:
    bool canAddTasks();

    void tryCreateNewChunk(ChunkPosition position);

    void startUpdatingChunk(Shared<Chunk> chunk);
    bool updateChunk(Shared<Chunk> chunk);

    // void handleChunk(Shared<Chunk> chunk);
    void handleChangingChunkStateTask(Shared<Chunk> chunk);
    void handleLoadedChunk(Shared<Chunk> chunk);
    void handleLazyChunk(Shared<Chunk> chunk);

    void runChunkBuild(Shared<Chunk> chunk);
    void runChunkProcessing(Shared<Chunk> chunk);
    void runChunkLoad(Shared<Chunk> chunk);
    void runChunkUnload(Shared<Chunk> chunk);

    void fireEventTick();
    void fireEventChunkUpdated(const Shared<Chunk>& chunk);
};

} // voxel

#endif //VOXEL_ENGINE_CHUNK_SOURCE_H

