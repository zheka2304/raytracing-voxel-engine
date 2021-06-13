
#ifndef VOXEL_ENGINE_GPU_CACHE_H
#define VOXEL_ENGINE_GPU_CACHE_H

#include <list>
#include <mutex>
#include <atomic>
#include <memory>
#include <glad/glad.h>

#include "common/types.h"


class GPUBufferPool {
private:
    static std::atomic<GLuint> lastContentUuid;

    struct BufferBase {
        GLuint glHandle;

        std::mutex ownsLock;
        GPUBufferPool* pool;
        size_t size = 0;
        content_uid_t contentUuid;

        BufferBase(GPUBufferPool* pool);
        void resize(size_t newSize);
        ~BufferBase();
    };

public:

    class Buffer {
        std::shared_ptr<BufferBase> handle;
        content_uid_t contentUuid = -1;

        Buffer(std::shared_ptr<BufferBase> handle, size_t size);

    public:
        Buffer();

        // tries to own buffer, if it was not owned from other place
        bool tryOwn();

        // releases buffer, so it can be reused in the future
        void release();

        // call when owned, marks contents were updated and stores new content id both to local field and shared handle
        void setContentUpdated();

        // call when owned, check that contents were unchanged since last setContentUpdated() call for this instance
        bool isContentUnchanged();

        // gets sync id of content stored in buffer
        content_uid_t getStoredContentUuid();

        // get sync id of last content, updated from this instance
        content_uid_t getLocalContentUuid();

        // returns gl buffer handle
        GLuint getGlHandle();

        friend GPUBufferPool;
    };

private:
    std::mutex poolLock;
    std::list<std::shared_ptr<BufferBase>> pool;

    std::atomic<size_t> totalMemoryUsed;
    size_t maxMemoryUsed;

    void _release(std::shared_ptr<BufferBase> buffer);

public:
    GPUBufferPool(size_t poolSize);

    // allocates new span of given size
    Buffer allocate(size_t size);

    friend BufferBase;
    friend Buffer;
};

#endif //VOXEL_ENGINE_GPU_CACHE_H
