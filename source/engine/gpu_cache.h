
#ifndef VOXEL_ENGINE_GPU_CACHE_H
#define VOXEL_ENGINE_GPU_CACHE_H

#include <list>
#include <mutex>
#include <atomic>
#include <memory>
#include <glad/glad.h>

class GPUBufferPool {
private:
    static std::atomic<GLuint> lastContentUuid;

    struct BufferBase {
        GLuint glHandle;

        std::mutex ownsLock;
        GPUBufferPool* pool;
        GLuint size = 0;
        GLuint contentUuid;

        BufferBase(GPUBufferPool* pool);
        void resize(GLuint newSize);
        ~BufferBase();
    };

public:

    class Buffer {
        std::shared_ptr<BufferBase> handle;
        GLuint contentUuid = -1;

        Buffer(std::shared_ptr<BufferBase> handle, GLuint size);

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
        GLuint getStoredContentUuid();

        // get sync id of last content, updated from this instance
        GLuint getLocalContentUuid();

        // returns gl buffer handle
        GLuint getGlHandle();

        friend GPUBufferPool;
    };

private:
    std::mutex poolLock;
    std::list<std::shared_ptr<BufferBase>> pool;

    std::atomic<GLuint> totalMemoryUsed;
    GLuint maxMemoryUsed;

    void _release(std::shared_ptr<BufferBase> buffer);

public:
    GPUBufferPool(GLuint poolSize);

    // allocates new span of given size
    Buffer allocate(GLuint size);

    friend BufferBase;
    friend Buffer;
};

#endif //VOXEL_ENGINE_GPU_CACHE_H
