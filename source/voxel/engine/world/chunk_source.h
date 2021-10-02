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
        // max loaded chunks updates per tick per tick
        i32 loaded_chunk_updates = 64;

        // Timeout and gpu memory ratio threshold, after which chunk will become lazy and unload from GPU memory.
        // For minimum gpu memory ration maximum timeout will be used and vice versa, so with increasing gpu memory chunks will be unloaded from gpu faster
        i32 chunk_lazy_timeout_min = 1000;
        i32 chunk_lazy_timeout_max = 2000;
        f32 chunk_lazy_gpu_mem_ratio_min = 0.1;
        f32 chunk_lazy_gpu_mem_ratio_max = 0.9;
    };

    // TODO: LoadingRegion related logic is not thread-safe
    class LoadingRegion {
        ChunkSource* m_chunk_source;
        math::Vec3i m_position;
        i32 m_min_size, m_max_size;

    public:
        LoadingRegion(ChunkSource* chunk_source, math::Vec3i position, i32 min_size, i32 max_size);
        LoadingRegion(const LoadingRegion& other) = delete;
        LoadingRegion(LoadingRegion&& other) = delete;

        math::Vec3i getPosition();
        void setPosition(math::Vec3i position);
        i32 getMaxSize();
        i32 getMinSize();

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
    Shared<threading::TaskExecutor> m_executor;

    ChunkSourceState m_state;
    std::vector<ChunkSourceListener*> m_listeners;
    Settings m_settings;
    f32 m_gpu_memory_ratio = 0;

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
                Shared<threading::TaskExecutor> executor,
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

    Shared<LoadingRegion> addLoadingRegion(math::Vec3i position, i32 min_size, i32 max_size);
    void removeLoadingRegion(Shared<LoadingRegion> loading_region);
    f32 getMinDistanceToLoadingRegion(ChunkPosition position, bool relative);

    void setGpuMemoryRatio(f32 ratio);
private:
    void runChunkTask(ChunkTask task);

    void tryCreateNewChunk(ChunkPosition position);
    void tryLoadLazyChunk(Shared<Chunk> chunk);
    void handleChangingChunkStateTask(Shared<Chunk> chunk);
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

