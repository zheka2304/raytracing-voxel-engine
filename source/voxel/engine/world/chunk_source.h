#ifndef VOXEL_ENGINE_CHUNK_SOURCE_H
#define VOXEL_ENGINE_CHUNK_SOURCE_H

#include <unordered_map>
#include <queue>
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

    struct Settings {
        // amount of chunk worker threads
        i32 worker_threads = 4;

        // max loaded chunks updates per tick per tick
        i32 loaded_chunk_updates = 64;

        // time since last fetch for chunk to be checked for changing state to lazy or start unloading
        i32 chunk_unload_timeout = 10000;
    };

    // TODO: LoadingRegion related logic is not thread-safe
    class LoadingRegion {
    public:
        static const i32 LEVEL_LOAD = 20;
        static const i32 LEVEL_LAZY = 18;

    private:
        ChunkSource* m_chunk_source;
        math::Vec3i m_position;
        i32 m_loading_level;

    public:
        LoadingRegion(ChunkSource* chunk_source, math::Vec3i position, i32 loading_level);
        LoadingRegion(const LoadingRegion& other) = delete;
        LoadingRegion(LoadingRegion&& other) = delete;

        math::Vec3i getPosition();
        void setPosition(math::Vec3i position);
        i32 getLoadingLevel();

        friend ChunkSource;
    };

private:
    enum ChunkTaskType {
        TASK_CREATE,
        TASK_LOAD
    };

    struct ChunkTask {
        ChunkPosition position;
        ChunkTaskType type;
        i64 priority;

        inline bool operator<(const ChunkTask& other) const {
            return priority < other.priority;
        }
    };

    Shared<ChunkProvider> m_provider;
    Shared<ChunkStorage> m_storage;

    ChunkSourceState m_state;
    std::vector<ChunkSourceListener*> m_listeners;
    Settings m_settings;

    std::mutex m_chunks_mutex;
    std::unordered_map<ChunkPosition, Shared<Chunk>> m_chunks;
    threading::BlockingQueue<Weak<Chunk>> m_updates_queue;

    threading::PriorityQueue<ChunkTask> m_chunk_task_queue;
    threading::ThreadPoolExecutor<ChunkTask> m_chunk_task_executor;

    std::mutex m_loaded_regions_mutex;
    std::vector<Shared<LoadingRegion>> m_loaded_regions;

public:
    ChunkSource(Shared<ChunkProvider> provider,
                Shared<ChunkStorage> storage,
                Settings settings,
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
    Shared<Chunk> fetchChunkAt(ChunkPosition position, i64 priority);

    Shared<LoadingRegion> addLoadingRegion(math::Vec3i position, i32 loading_level);
    void removeLoadingRegion(Shared<LoadingRegion> loading_region);
    i32 getLoadingLevelForPosition(ChunkPosition position);

private:
    void runChunkTask(ChunkTask task);

    void tryCreateNewChunk(ChunkPosition position);
    void tryLoadLazyChunk(Shared<Chunk> chunk);
    void handleChunkLoading(Shared<Chunk> chunk);
    void handleLazyChunk(Shared<Chunk> chunk);
    void startUpdatingChunk(Shared<Chunk> chunk);
    bool updateChunk(Shared<Chunk> chunk);

    void runChunkBuild(Shared<Chunk> chunk);
    void runChunkProcessing(Shared<Chunk> chunk);
    void runChunkLoad(Shared<Chunk> chunk);
    void runChunkUnload(Shared<Chunk> chunk);

    void fireEventTick();
    void fireEventChunkUpdated(const Shared<Chunk>& chunk);
};

} // voxel

#endif //VOXEL_ENGINE_CHUNK_SOURCE_H

