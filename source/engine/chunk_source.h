
#ifndef VOXEL_ENGINE_CHUNK_SOURCE_H
#define VOXEL_ENGINE_CHUNK_SOURCE_H

#include <mutex>
#include <atomic>
#include <unordered_map>
#include <list>
#include <functional>
#include <thread>

#include "common/chunk_pos.h"
#include "common/queue.h"
#include "voxel_chunk.h"


// chunk source interface provides access to voxel chunks
class ChunkSource {
public:
    // returns chunk for position, upon normal conditions must not
    // block the thread, as it is used in rendering thread to attach and detach render chunks
    virtual VoxelChunk* getChunkAt(ChunkPos const& pos) = 0;

    // calling this function should force async chunk load,
    // this function should not be called every frame on every chunk, as for some implementations it may
    // take some time to queue task
    virtual void requestChunk(ChunkPos const& pos);

    virtual ~ChunkSource();
};


// chunk source for debugging, has chunk provider function,
// caches all non-null chunks for faster access in the future
// do not unload chunks
class DebugChunkSource : public ChunkSource {
public:
    std::unordered_map<ChunkPos, VoxelChunk*> chunkMap;
    std::function<VoxelChunk*(ChunkPos)> chunkProviderFunc;

    explicit DebugChunkSource(std::function<VoxelChunk*(ChunkPos)> providerFunc);
    VoxelChunk* getChunkAt(ChunkPos const& pos) override;
    ~DebugChunkSource() override;
};


// Allows to run multi-threaded tasks with region locks,
// runs up to certain amount of worker threads.
// Has separate support for generation and rebuild tasks
class ThreadedChunkSource : public ChunkSource {
public:
    struct Task {
        // main logic of the task
        std::function<void()> task;
        // pre check can be added to discard task before locking it
        std::function<bool()> preCheck = [] () -> bool { return true; };
        // list of chunk positions to be locked
        std::list<ChunkPos> regionLock;
        // if lock failed, should task be discarded of re-added at the end of task queue
        bool discardIfLockFailed = false;

        Task(std::function<void()> task, std::list<ChunkPos>&& regionLock, bool discardIfLockFailed);
        Task(std::function<void()> task, std::function<bool()> preCheck, std::list<ChunkPos>&& regionLock, bool discardIfLockFailed);
        void run() const;
    };

private:
    static const int MAX_CHUNK_LOCKS = 64;

    struct ChunkLock {
    private:
        std::unique_lock<std::mutex> lock;
        std::mutex _mutex;

    public:
        ChunkLock();
        inline bool try_lock() { return lock.try_lock(); }
        inline void unlock() { lock.unlock(); }
        inline bool owns() const { return lock.owns_lock(); }
    };

    struct RegionLock {
    private:
        bool _owns;
        std::unordered_map<ChunkPos, ChunkLock*> _ownedChunks;
    public:
        RegionLock(std::unordered_map<ChunkPos, ChunkLock*>&& ownedChunks, bool owns);
        explicit RegionLock(bool owns);
        bool owns() const;
        void unlock();
    };

    std::atomic<bool> isDestroyPending = false;
    std::vector<std::thread> workerThreads;
    BlockingQueue<Task> taskQueue;

    std::mutex lockedChunksMutex;
    std::unordered_map<ChunkPos, ChunkLock> lockedChunks;
    std::mutex chunkMapMutex;
    std::unordered_map<ChunkPos, VoxelChunk*> chunkMap;

    std::function<bool(VoxelChunk* chunk)> defaultGenerationTask;

public:
    ThreadedChunkSource(std::function<bool(VoxelChunk* chunk)> defaultGenerationTask, int maxWorkerThreads);
    void addTask(Task const& task);
    void releaseExcessChunkLocks();
    ~ThreadedChunkSource() override;

    VoxelChunk* getChunkAt(ChunkPos const& pos) override;
    void putChunkAt(ChunkPos const& pos, VoxelChunk* chunk);
    void removeChunkAt(ChunkPos const& pos);

    void requestChunk(const ChunkPos &pos) override;
    void addGenerationTask(ChunkPos const& pos, std::function<bool(VoxelChunk* chunk)> generator, int lockRadiusXZ = 0, int lockRadiusY = 0);
private:
    RegionLock tryLockRegion(std::list<ChunkPos> const& regionLock);
    void threadLoop();
};

#endif //VOXEL_ENGINE_CHUNK_SOURCE_H
