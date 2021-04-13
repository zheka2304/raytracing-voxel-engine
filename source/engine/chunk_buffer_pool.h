
#ifndef VOXEL_ENGINE_CHUNK_BUFFER_POOL_H
#define VOXEL_ENGINE_CHUNK_BUFFER_POOL_H

#include <glad/glad.h>
#include <atomic>
#include <mutex>
#include <list>
#include <unordered_map>

#include "gpu_cache.h"

class VoxelChunk;
class BakedChunkBuffer;


// Implements GPU cache for non-baked chunk data, allows maximum allocation of certain size
// this cache is passed to baking compute shader
class PooledChunkBuffer {
public:
    static const int MAX_POOLED_MEMORY_SIZE = 1024 * 1024 * 1024; // 256 MB

private:
    VoxelChunk* chunk;
    GPUBufferPool::Buffer buffer;

    int lastSyncId = 1;
    int lastUpdateId = 0;

    static GPUBufferPool bufferPool;
public:
    explicit PooledChunkBuffer(VoxelChunk* chunk);
    int getBufferSize();
    GLuint ownHandle();
    void releaseHandle();
    void update();
    ~PooledChunkBuffer();

private:
    void _sync(GLuint handle);
};


// Holds baked chunk data, either in shared GPU buffer or on CPU memory,
// if GPU buffer span was replaced by another chunk
class BakedChunkBuffer {
private:
    GLuint sharedBufferHandle = 0;
    int sharedBufferOffset;
    int bufferSize;

    unsigned int* cacheBuffer = 0;
    int cacheBufferSize = 0;
    int64_t cacheSyncUuid = -1;
    int64_t bakeSyncUuid = -1;

public:
    // own shared buffer span, use cache or run baking compute shader, release cache if out of cache memory
    void bake(PooledChunkBuffer& chunkBuffer, GLuint sharedBufferHandle, GLuint sharedBufferOffset);
    // releases shared buffer, creates cache, if able to do it
    void release();
    void releaseAndDestroyCache();
    // check, if owns shared buffer span
    bool owns();
    ~BakedChunkBuffer();
};


#endif //VOXEL_ENGINE_CHUNK_BUFFER_POOL_H
