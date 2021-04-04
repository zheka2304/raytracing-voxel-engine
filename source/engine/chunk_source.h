
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
#include "chunk_handler.h"


// Chunk source interface provides access to voxel chunks
class ChunkSource {
public:
    // returns loaded chunk for position, upon normal conditions must not
    // block the thread, as it is used in rendering thread to attach and detach render chunks
    // if no chunk exist or loaded returns null
    virtual VoxelChunk* getChunkAt(ChunkPos const& pos) = 0;

    // calling this function should force async chunk load,
    // this function should not be called every frame on every chunk, as for some implementations it may
    // take some time to queue task
    virtual void requestChunk(ChunkPos const& pos);

    // should be called, when all chunks were requested,
    // and must in background unload chunks, that were not requested for a while
    virtual void startUnload();

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

    struct ChunkContainer {
    public:
        VoxelChunk* chunk = nullptr;
        unsigned long long lastRequestTime;
    };

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

    std::atomic<bool> isChunkSourceValid = true;
    std::atomic<bool> isDestroyPending = false;
    std::vector<std::thread> workerThreads;
    BlockingQueue<Task> taskQueue;

    std::mutex lockedChunksMutex;
    std::unordered_map<ChunkPos, ChunkLock> lockedChunks;
    std::mutex chunkMapMutex;
    std::unordered_map<ChunkPos, ChunkContainer> chunkMap;

    std::shared_ptr<ChunkHandler> chunkHandler;

public:
    ThreadedChunkSource(std::shared_ptr<ChunkHandler> chunkHandler, int maxWorkerThreads);
    void addTask(Task const& task);
    void releaseExcessChunkLocks();
    ~ThreadedChunkSource() override;

public:
    VoxelChunk* getChunkAt(ChunkPos const& pos) override;

private:
    VoxelChunk* requestExistingChunkAt(ChunkPos const& pos, bool updateRequestTime);
    void putChunkAt(ChunkPos const& pos, VoxelChunk* chunk);
    void removeChunkAt(ChunkPos const& pos);

    bool canChunkBeRequested(ChunkPos const& pos, bool updateRequestTime);
    bool canChunkBeBaked(ChunkPos const& pos);

public:
    // this will queue task of requesting and than, after this is succeeded, baking chunk
    void requestChunk(const ChunkPos &pos) override;
    //
    void requestChunkBaking(const ChunkPos &pos);
    //
    void startUnload() override;

    // use this before destroying chunk source to make all threads run unload tasks and join
    // this will invalidate chunk source
    void unloadAllImmediately();

private:
    RegionLock tryLockRegion(std::list<ChunkPos> const& regionLock);
    void threadLoop();

    static inline unsigned long long getCurrentTime() {
        using namespace std::chrono;
        milliseconds ms = duration_cast< milliseconds >(
                system_clock::now().time_since_epoch()
        );
        return ms.count();
    }

public:
    static std::list<ChunkPos> makeRegionLock(ChunkPos pos, int xzRadius, int yRadius);
};

#endif //VOXEL_ENGINE_CHUNK_SOURCE_H
