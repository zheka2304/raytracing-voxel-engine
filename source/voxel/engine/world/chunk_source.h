#ifndef VOXEL_ENGINE_CHUNK_SOURCE_H
#define VOXEL_ENGINE_CHUNK_SOURCE_H

#include <unordered_map>
#include <queue>
#include <mutex>

#include "voxel/common/base.h"
#include "voxel/common/threading.h"
#include "voxel/engine/world/chunk.h"
#include "voxel/engine/world/chunk_lock.h"
#include "voxel/engine/world/chunk_provider.h"


namespace voxel {

class ChunkStorage;
class ChunkProvider;
class ChunkSource;

class ChunkSourceListener {
public:
    virtual void onChunkSourceTick(ChunkSource& chunk_source);
    virtual void onChunkUpdated(ChunkSource& chunk_source, ChunkRef chunk_ref);
};

enum ChunkAccessPolicy {
    // lock chunk on access, fail only when no chunk exist
    chunk_access_policy_strong = 1,

    // try lock chunk on access, fail, in case it cannot be locked or no chunk exist
    chunk_access_policy_weak,

    // lock the map instead of locking the chunk
    chunk_access_policy_map_only,

    // don't lock chunk at all (however it still locks the map, when querying position, use with extreme caution!)
    chunk_access_policy_no_lock
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

        // max loaded chunks updates per tick
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

    Unique<ChunkProvider> m_provider;
    Unique<ChunkStorage> m_storage;

    ChunkSourceState m_state;
    std::vector<ChunkSourceListener*> m_listeners;
    Settings m_settings;

    std::mutex m_chunks_mutex;
    flat_hash_map<ChunkPosition, Unique<Chunk>> m_chunks;
    threading::BlockingQueue<ChunkRef> m_updates_queue;

    threading::PriorityQueue<ChunkTask> m_chunk_task_queue;
    threading::ThreadPoolExecutor<ChunkTask> m_chunk_task_executor;

    std::mutex m_loaded_regions_mutex;
    std::vector<Shared<LoadingRegion>> m_loaded_regions;

public:
    ChunkSource(Unique<ChunkProvider> provider,
                Unique<ChunkStorage> storage,
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

    // access and lock chunk according to given policy, on success, acquire will be called, otherwise - fallback, will return true on success
    template<ChunkAccessPolicy policy, typename AcquireFunc, typename FallbackFunc>
    inline bool accessChunk(const ChunkRef& ref, AcquireFunc acquire, FallbackFunc fallback) {
        ThreadLock map_lock(m_chunks_mutex);
        if (auto it = m_chunks.find(ref.position()); it != m_chunks.end()) {
            Chunk* chunk = it->second.get();
            if constexpr(policy == chunk_access_policy_strong) {
                ThreadLock chunk_lock(chunk->getLock());
                map_lock.unlock();
                acquire(*chunk);
                chunk_lock.unlock();
                return true;
            } else if constexpr(policy == chunk_access_policy_weak) {
                if (chunk->tryLock()) {
                    map_lock.unlock();
                    acquire(*chunk);
                    chunk->unlock();
                    return true;
                } else {
                    map_lock.unlock();
                }
            } else if constexpr(policy == chunk_access_policy_map_only) {
                acquire(*chunk);
                map_lock.unlock();
                return true;
            } else if constexpr(policy == chunk_access_policy_no_lock) {
                map_lock.unlock();
                acquire(*chunk);
                return true;
            }
            fallback(true);
        } else {
            fallback(false);
        }
        return false;
    }

    template<ChunkAccessPolicy policy, typename AcquireFunc>
    inline bool accessChunk(const ChunkRef& ref, AcquireFunc acquire) {
        return accessChunk<policy>(ref, acquire, [] (bool) {});
    }

    template<typename AcquireFunc, typename FallbackFunc>
    bool fetchChunkAt(ChunkPosition position, i64 priority, AcquireFunc acquire, FallbackFunc fallback) {
        return accessChunk<chunk_access_policy_weak>(ChunkRef(position), [&](Chunk& chunk) {
            chunk.fetch();
            auto chunk_state = chunk.getState();
            if (chunk_state == CHUNK_LOADED) {
            } else if (chunk_state == CHUNK_LAZY) {
                tryLoadLazyChunk(chunk);
            } else {
                m_chunk_task_queue.push({ position, TASK_LOAD, priority });
            }
            acquire(chunk);
        }, [&] (bool exists) {
            if (m_provider->canFetchChunk(*this, position)) {
                m_chunk_task_queue.push({ position, TASK_CREATE, priority });
            }
            fallback(exists);
        });
    }

    template<typename AcquireFunc>
    inline bool fetchChunkAt(ChunkPosition position, i64 priority, AcquireFunc acquire) {
        return fetchChunkAt(position, priority, acquire, [] (bool) {});
    }

    const Shared<LoadingRegion>& addLoadingRegion(math::Vec3i position, i32 loading_level);
    void removeLoadingRegion(const Shared<LoadingRegion>& loading_region);
    i32 getLoadingLevelForPosition(ChunkPosition position);

private:
    void runChunkTask(ChunkTask task);

    void tryCreateNewChunk(ChunkPosition position);
    void tryLoadLazyChunk(Chunk& chunk);
    void handleChunkLoading(Chunk& chunk);
    void handleLazyChunk(Chunk& chunk);
    void startUpdatingChunk(Chunk& chunk);
    bool updateChunk(const ChunkRef& ref);

    void runChunkBuild(Chunk& chunk);
    void runChunkProcessing(Chunk& chunk);
    void runChunkLoad(Chunk& chunk);
    void runChunkUnload(Chunk& chunk);

    void fireEventTick();
    void fireEventChunkUpdated(Chunk& chunk);
};

} // voxel

#endif //VOXEL_ENGINE_CHUNK_SOURCE_H

