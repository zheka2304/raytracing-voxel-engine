#include "chunk_source.h"

#include <utility>
#include <iostream>


ChunkSource::~ChunkSource() = default;

void ChunkSource::requestChunk(const ChunkPos &pos) {

}

DebugChunkSource::DebugChunkSource(std::function<VoxelChunk *(ChunkPos)> providerFunc) : chunkProviderFunc(std::move(providerFunc)) {

}

VoxelChunk* DebugChunkSource::getChunkAt(ChunkPos const& pos) {
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

ThreadedChunkSource::ChunkLock::ChunkLock() : lock(_mutex, std::defer_lock) {

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


ThreadedChunkSource::ThreadedChunkSource(std::function<bool(VoxelChunk* chunk)> defaultGenerationTask, int maxWorkerThreads) : defaultGenerationTask(std::move(defaultGenerationTask)) {
    for (int i = 0; i < maxWorkerThreads; i++) {
        workerThreads.emplace_back(std::thread(&ThreadedChunkSource::threadLoop, this));
    }
}

void ThreadedChunkSource::threadLoop() {
    while (!isDestroyPending) {
        Task task = taskQueue.pop();
        std::cout << "got task\n";
        if (!task.preCheck()) {
            continue;
        }
        std::cout << "check success\n";
        RegionLock lock = tryLockRegion(task.regionLock);
        if (lock.owns()) {
            std::cout << "locked\n";
            task.run();
            std::cout << "completed\n";
            lock.unlock();
            std::cout << "unlocked\n";
            releaseExcessChunkLocks();
        } else if (!task.discardIfLockFailed) {
            std::cout << "re-add\n";
            addTask(task);
        }
    }
}

void ThreadedChunkSource::addTask(Task const& task) {
    taskQueue.push(task);
}

ThreadedChunkSource::RegionLock ThreadedChunkSource::tryLockRegion(std::list<ChunkPos> const& regionLock) {
    if (regionLock.empty()) {
        return RegionLock(true);
    }
    std::lock_guard<std::mutex> lockGuard(lockedChunksMutex);
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

void ThreadedChunkSource::releaseExcessChunkLocks() {
    std::lock_guard<std::mutex> lockGuard(lockedChunksMutex);
    if (lockedChunks.size() >= MAX_CHUNK_LOCKS) {
        for (auto it = lockedChunks.begin(); it != lockedChunks.end();) {
            if (!it->second.owns()) {
                it = lockedChunks.erase(it);
            } else {
                it++;
            }
        }
    }
}

VoxelChunk* ThreadedChunkSource::getChunkAt(const ChunkPos &pos) {
    std::lock_guard<std::mutex> lockGuard(chunkMapMutex);
    auto it = chunkMap.find(pos);
    if (it != chunkMap.end()) {
        return it->second;
    }
    return nullptr;
}

void ThreadedChunkSource::putChunkAt(const ChunkPos &pos, VoxelChunk* chunk) {
    std::lock_guard<std::mutex> lockGuard(chunkMapMutex);
    auto it = chunkMap.find(pos);
    if (it != chunkMap.end()) {
        delete(it->second);
        it->second = chunk;
    } else {
        chunkMap.emplace(pos, chunk);
    }
}

void ThreadedChunkSource::removeChunkAt(const ChunkPos &pos) {
    std::lock_guard<std::mutex> lockGuard(chunkMapMutex);
    auto it = chunkMap.find(pos);
    if (it != chunkMap.end()) {
        delete(it->second);
        chunkMap.erase(it);
    }
}

void ThreadedChunkSource::requestChunk(const ChunkPos &pos) {
    if (pos.y != 0) return;
    addGenerationTask(pos, defaultGenerationTask);
}

void ThreadedChunkSource::addGenerationTask(const ChunkPos &pos, std::function<bool(VoxelChunk*)> generator,
                                            int lockRadiusXZ, int lockRadiusY) {
    // if chunk is already generated, do not queue task
    VoxelChunk* preQueueCheckChunk = getChunkAt(pos);
    if (preQueueCheckChunk != nullptr && preQueueCheckChunk->state != VoxelChunk::STATE_INITIALIZED) {
        return;
    }

    std::list<ChunkPos> regionLock;
    for (int y = -lockRadiusY; y <= lockRadiusY; y++) {
        for (int x = -lockRadiusXZ; x <= lockRadiusXZ; x++) {
            for (int z = -lockRadiusXZ; z <= lockRadiusXZ; z++) {
                regionLock.emplace_back(ChunkPos(pos.x + x, pos.y + y, pos.z + z));
            }
        }
    }

    addTask(Task([=] () -> void {
        std::cout << "run task\n";
        VoxelChunk* existingChunk = getChunkAt(pos);
        if (existingChunk != nullptr && existingChunk->state != VoxelChunk::STATE_INITIALIZED) {
            return;
        }

        VoxelChunk* chunk = existingChunk != nullptr ? existingChunk : new VoxelChunk();
        chunk->setChunkSource(this);
        chunk->setPos(pos);
        putChunkAt(pos, chunk);
        if (generator(chunk)) {
            chunk->setState(VoxelChunk::STATE_GENERATED);
        } else {
            removeChunkAt(pos);
        }
        std::cout << "completed task\n";
    }, [&] () -> bool {
        VoxelChunk* existingChunk = getChunkAt(pos);
        return existingChunk == nullptr || existingChunk->state == VoxelChunk::STATE_INITIALIZED;
    }, std::move(regionLock), false));
}


ThreadedChunkSource::~ThreadedChunkSource() {
    // mark chunk source destroyed and clear queue
    isDestroyPending = true;
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
        delete(posAndChunk.second);
    }
}