
#ifndef VOXEL_ENGINE_CHUNK_BUFFERS_H
#define VOXEL_ENGINE_CHUNK_BUFFERS_H

#include <glad/glad.h>
#include <atomic>
#include <mutex>
#include <list>
#include <unordered_map>
#include <vector>

#include "common/types.h"
#include "common/vec.h"
#include "gpu_cache.h"

class VoxelChunk;


// Implements GPU cache for non-baked chunk data, allows maximum allocation of certain size
// this cache is passed to baking compute shader
class PooledChunkBuffer {
private:
    VoxelChunk* chunk;
    GPUBufferPool::Buffer buffer;

    content_uid_t lastSyncId = 1;
    content_uid_t lastUpdateId = 0;

    static GPUBufferPool bufferPool;
public:
    explicit PooledChunkBuffer(VoxelChunk* chunk);
    size_t getBufferSize();
    content_uid_t getStoredContentUuid();
    GLuint ownHandle();
    void releaseHandle();
    void update();
    ~PooledChunkBuffer();

private:
    void _sync(GLuint handle, int maxLockDelayMilliseconds = 250);
};


// Holds baked chunk data
class BakedChunkBuffer {
private:
    GLuint sharedBufferHandle = 0;
    size_t sharedBufferOffset, sharedBufferSpanSize;

    // currently owned chunk buffer stored content uuid
    content_uid_t ownedContentUuid = 0;

    // lookup map by stored chunk content uuid
    static std::mutex cacheByContentIdMapMutex;
    static std::unordered_map<content_uid_t, GPUBufferPool::Buffer> cacheByContentIdMap;

    // cache buffer pool
    static GPUBufferPool bufferPool;

public:
    // own shared buffer span, use cache or run baking compute shader, release cache if out of cache memory
    void bake(PooledChunkBuffer& chunkBuffer, GLuint sharedBufferHandle, size_t sharedBufferOffset, std::vector<Vec3i> const& regions);
    // releases shared buffer, creates cache, if able to do it
    void release();
    void releaseAndDestroyCache();
    ~BakedChunkBuffer();

private:
    void _dispatchCompute(GLuint chunkBufferHandle, Vec3i offset, Vec3i size);
};


#endif //VOXEL_ENGINE_CHUNK_BUFFERS_H
