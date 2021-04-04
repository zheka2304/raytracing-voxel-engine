#include "chunk_source.h"

#include <utility>
#include <iostream>


ChunkSource::~ChunkSource() = default;

void ChunkSource::requestChunk(const ChunkPos &pos) {

}

void ChunkSource::startUnload() {

}


DebugChunkSource::DebugChunkSource(std::function<VoxelChunk *(ChunkPos)> providerFunc) : chunkProviderFunc(std::move(providerFunc)) {

}

VoxelChunk* DebugChunkSource::getChunkAt(ChunkPos const& pos) {
    if (pos.y != 0) return nullptr;

    auto it = chunkMap.find(pos);
    if (it != chunkMap.end()) {
        return it->second;
    }
    VoxelChunk* chunk = chunkProviderFunc(pos);
    if (chunk != nullptr) {
        chunk->chunkSource = this;
        chunk->setPos(pos);
        chunkMap.emplace(pos, chunk);
    }
    return chunk;
}

DebugChunkSource::~DebugChunkSource() {
    for (auto posAndChunk : chunkMap) {
        delete(posAndChunk.second);
    }
}



ThreadedChunkSource::Task::Task(std::function<void()> task, std::list<ChunkPos>&& regionLock, bool discardIfLockFailed) :
        task(std::move(task)), regionLock(regionLock), discardIfLockFailed(discardIfLockFailed) {

}


ThreadedChunkSource::Task::Task(std::function<void()> task, std::function<bool()> preCheck, std::list<ChunkPos>&& regionLock, bool discardIfLockFailed) :
        task(std::move(task)), preCheck(std::move(preCheck)), regionLock(regionLock), discardIfLockFailed(discardIfLockFailed) {

}

void ThreadedChunkSource::Task::run() const {
    task();
}

ThreadedChunkSource::RegionLock::RegionLock(std::unordered_map<ChunkPos, ChunkLock*> &&ownedLocks, bool owns) : _ownedChunks(ownedLocks), _owns(owns) {

}

ThreadedChunkSource::RegionLock::RegionLock(bool owns) : _owns(owns) {

}

bool ThreadedChunkSource::RegionLock::owns() const {
    return _owns;
}

void ThreadedChunkSource::RegionLock::unlock() {
    _owns = false;
    for (auto const& posAndLock : _ownedChunks) {
        posAndLock.second->unlock();
    }
}

ThreadedChunkSource::ChunkLock::~ChunkLock() {
    if (lock != nullptr) {
        std::cout << "destroying not released ChunkLock, this will cause it to wait unlock\n";
        lock->lock();
        std::mutex* _lock = lock;
        lock = nullptr;
        _lock->unlock();
        delete(_lock);
    }
}


ThreadedChunkSource::ThreadedChunkSource(std::shared_ptr<ChunkHandler> chunkHandler, int maxWorkerThreads, int minIdleTimeToUnload) :
    chunkHandler(std::move(chunkHandler)), minIdleTimeToUnload(minIdleTimeToUnload) {
    for (int i = 0; i < maxWorkerThreads; i++) {
        workerThreads.emplace_back(std::thread(&ThreadedChunkSource::threadLoop, this));
    }
}

void ThreadedChunkSource::threadLoop() {
    while (!isDestroyPending) {
        Task task = taskQueue.pop();
        if (!task.preCheck()) {
            continue;
        }
        RegionLock lock = tryLockRegion(task.regionLock);
        if (lock.owns()) {
            task.run();
            unlockRegion(lock);
            releaseExcessChunkLocks();
        } else if (!task.discardIfLockFailed) {
            addTask(task);
        }
    }
}

void ThreadedChunkSource::addTask(Task const& task) {
    if (isChunkSourceValid) {
        taskQueue.push(task);
    }
}

ThreadedChunkSource::RegionLock ThreadedChunkSource::tryLockRegion(std::list<ChunkPos> const& regionLock) {
    if (regionLock.empty()) {
        return RegionLock(true);
    }
    std::unique_lock<std::mutex> lock(lockedChunksMutex);
    std::unordered_map<ChunkPos, ChunkLock*> ownedChunks;
    for (ChunkPos const& pos : regionLock) {
        ChunkLock& chunkLock = lockedChunks[pos];
        if (chunkLock.try_lock()) {
            ownedChunks.emplace(pos, &chunkLock);
        } else {
            for (auto const& posAndMutex : ownedChunks) {
                posAndMutex.second->unlock();
            }
            return RegionLock(false);
        }
    }
    return RegionLock(std::move(ownedChunks), true);
}

void ThreadedChunkSource::unlockRegion(RegionLock& regionLock) {
    std::unique_lock<std::mutex> lock(lockedChunksMutex);
    regionLock.unlock();
}

void ThreadedChunkSource::releaseExcessChunkLocks() {
    std::unique_lock<std::mutex> lock(lockedChunksMutex);
    if (lockedChunks.size() >= MAX_CHUNK_LOCKS) {
        for (auto it = lockedChunks.begin(); it != lockedChunks.end();) {
            if (it->second.try_release()) {
                it = lockedChunks.erase(it);
            } else {
                it++;
            }
        }
    }
}

VoxelChunk* ThreadedChunkSource::getChunkAt(const ChunkPos &pos) {
    return requestExistingChunkAt(pos, false);
}

VoxelChunk* ThreadedChunkSource::requestExistingChunkAt(const ChunkPos &pos, bool updateRequestTime) {
    std::unique_lock<std::mutex> lock(chunkMapMutex);
    auto it = chunkMap.find(pos);
    if (it != chunkMap.end()) {
        if (updateRequestTime) {
            it->second.lastRequestTime = getCurrentTime();
        }
        return it->second.chunk;
    }
    return nullptr;
}

void ThreadedChunkSource::putChunkAt(ChunkPos const& pos, VoxelChunk* chunk) {
    std::unique_lock<std::mutex> lock(chunkMapMutex);
    auto it = chunkMap.find(pos);
    chunk->setChunkSource(this);
    chunk->setPos(pos);
    if (it != chunkMap.end()) {
        delete(it->second.chunk);
        it->second.chunk = chunk;
    } else {
        chunkMap.emplace(pos, ChunkContainer({ chunk, getCurrentTime() }));
    }
}

void ThreadedChunkSource::removeChunkAt(const ChunkPos &pos) {
    std::unique_lock<std::mutex> lock(chunkMapMutex);
    auto it = chunkMap.find(pos);
    if (it != chunkMap.end()) {
        delete(it->second.chunk);
        chunkMap.erase(it);
    }
}

bool ThreadedChunkSource::canChunkBeRequested(ChunkPos const& pos, bool updateRequestTime) {
    VoxelChunk* chunk = requestExistingChunkAt(pos, updateRequestTime);
    return chunk == nullptr || chunk->state == VoxelChunk::STATE_INITIALIZED;
}

bool ThreadedChunkSource::canChunkBeBaked(ChunkPos const& pos) {
    VoxelChunk* chunk = requestExistingChunkAt(pos, false);
    return chunk != nullptr && chunk->state == VoxelChunk::STATE_POPULATED;
}

void ThreadedChunkSource::requestChunk(ChunkPos const& pos) {
    // check position is valid
    if (!chunkHandler->isValidPosition(pos)) return;

    // before adding task, quickly check, if it was already requested
    if (!canChunkBeRequested(pos, true)) {
        return;
    }

    Vec2i lockRadius = chunkHandler->getRequestLockRadius();
    // add task
    addTask(Task(
            [this, pos] () -> void {
                // after task is locked, check last time, if chunk can be requested
                // now it cant change, because only this task will run for this chunk
                if (!canChunkBeRequested(pos, true)) {
                    return;
                }

                // get chunk, if it is not existing, create it
                VoxelChunk* chunk = getChunkAt(pos);
                if (chunk == nullptr) {
                    chunk = new VoxelChunk();
                    putChunkAt(pos, chunk);
                }

                // run chunk handler request task
                // if chunk is successfully requested by handler, populate it
                if (chunkHandler->requestChunk(*chunk)) {
                    chunk->state = VoxelChunk::STATE_POPULATED;
                    requestChunkBaking(pos);
                }
                // if chunk was not requested successfully, it will be requested again
                // or unloaded after some time, so just leave it and dont delete
            },
            // pre check before locking, if chunk still can be requested
            [this, pos] () -> bool { return canChunkBeRequested(pos, false); },
            // move lock to task
            makeRegionLock(pos, lockRadius.x, lockRadius.y),
            // if lock fails, just drop the task, more requests will be sent
            true
    ));
}

void ThreadedChunkSource::requestChunkBaking(ChunkPos const& pos) {
    // before adding task, quickly check, if it was already requested
    if (!canChunkBeBaked(pos)) {
        return;
    }

    Vec2i lockRadius = chunkHandler->getBakeLockRadius();
    // add task
    addTask(Task(
            [this, pos] () -> void {
                if (!canChunkBeBaked(pos)) {
                    return;
                }

                // get chunk, if it is not existing, create it
                VoxelChunk* chunk = requestExistingChunkAt(pos, true);
                if (chunk == nullptr) {
                    return;
                }

                // run chunk handler request task
                // if chunk is successfully requested by handler, populate it
                chunkHandler->bakeChunk(*chunk);
                chunk->state = VoxelChunk::STATE_BAKED;
            },
            // pre check before locking, if chunk still can be requested
            [this, pos] () -> bool { return canChunkBeBaked(pos); },
            // move lock to task
            makeRegionLock(pos, lockRadius.x, lockRadius.y),
            // bake task is added once chunk is requested, so it must complete
            false
    ));
}

void ThreadedChunkSource::startUnload() {
    unsigned long long unloadTime = getCurrentTime() - minIdleTimeToUnload;
    std::unique_lock<std::mutex> lock(chunkMapMutex);
    for (auto const& posAndChunk : chunkMap) {
        if (posAndChunk.second.lastRequestTime < unloadTime) {
            ChunkPos pos = posAndChunk.first;
            addTask(Task([pos, this] () -> void {
                // check if chunk still must be unloaded
                std::unique_lock<std::mutex> lock(chunkMapMutex);
                auto it = chunkMap.find(pos);
                if (it != chunkMap.end()) {
                    if (it->second.lastRequestTime > getCurrentTime() - minIdleTimeToUnload) {
                        return;
                    }
                } else {
                    return;
                }
                lock.unlock();

                // unload chunk
                VoxelChunk* chunk = getChunkAt(pos);
                if (chunk != nullptr) {
                    chunkHandler->unloadChunk(*chunk);
                    removeChunkAt(pos);
                }
            }, std::list<ChunkPos>({ pos }), true));
        }
    }
}

std::list<ChunkPos> ThreadedChunkSource::makeRegionLock(ChunkPos pos, int xzRadius, int yRadius) {
    std::list<ChunkPos> regionLock;
    for (int y = -yRadius; y <= yRadius; y++) {
        for (int x = -xzRadius; x <= xzRadius; x++) {
            for (int z = -xzRadius; z <= xzRadius; z++) {
                regionLock.emplace_back(ChunkPos(pos.x + x, pos.y + y, pos.z + z));
            }
        }
    }
    return regionLock;
}


void ThreadedChunkSource::unloadAllImmediately() {
    // invalidate chunk source to block adding new tasks
    isChunkSourceValid = false;
    taskQueue.clear();

    // manually queue unload tasks for every chunk
    std::unique_lock<std::mutex> lock(chunkMapMutex);
    for (auto const& posAndChunk : chunkMap) {
        VoxelChunk* chunk = posAndChunk.second.chunk;
        std::list<ChunkPos> regionLock; regionLock.emplace_back(posAndChunk.first);
        taskQueue.push(Task([=] () -> void { chunkHandler->unloadChunk(*chunk); removeChunkAt(chunk->position); }, std::move(regionLock), false));
    }
    lock.unlock();

    // wait all threads to join
    for (std::thread& thread : workerThreads) {
        thread.join();
    }
}

ThreadedChunkSource::~ThreadedChunkSource() {
    // mark chunk source destroyed and clear queue
    isDestroyPending = true;
    isChunkSourceValid = false;
    taskQueue.clear();
    // add empty task for each thread, to assure, that queue wont be blocked
    // each thread will consume exactly one task and then check and exist
    for (std::thread& thread : workerThreads) {
        addTask(Task([] () -> void {}, std::list<ChunkPos>(), false));
    }
    // join every thread to complete current task for each one
    for (std::thread& thread : workerThreads) {
        thread.join();
    }
    // delete all chunks
    for (auto const& posAndChunk : chunkMap) {
        delete(posAndChunk.second.chunk);
    }
}