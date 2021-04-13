

#include "gpu_cache.h"

std::atomic<GLuint> GPUBufferPool::lastContentUuid = 1;

GPUBufferPool::BufferBase::BufferBase(GPUBufferPool* pool) : pool(pool) {
    contentUuid = GPUBufferPool::lastContentUuid++;
    glGenBuffers(1, &glHandle);
}

GPUBufferPool::BufferBase::~BufferBase() {
    glDeleteBuffers(1, &glHandle);
}

GPUBufferPool::Buffer::Buffer(std::shared_ptr<BufferBase> handle, GLuint size) : handle(handle) {
    handle->size = size;
}

GPUBufferPool::Buffer::Buffer() : handle(nullptr) {

}

bool GPUBufferPool::Buffer::tryOwn() {
    if (handle != nullptr && handle->ownsLock.try_lock()) {
        return true;
    }
    return false;
}

void GPUBufferPool::Buffer::release() {
    if (handle != nullptr) {
        handle->ownsLock.unlock();
        handle->pool->_release(handle);
    }
}

void GPUBufferPool::Buffer::setContentUpdated() {
    if (handle != nullptr) {
        contentUuid = handle->contentUuid = GPUBufferPool::lastContentUuid++;
    }
}

bool GPUBufferPool::Buffer::isContentUnchanged() {
    return handle != nullptr && handle->contentUuid == contentUuid;
}

GLuint GPUBufferPool::Buffer::getGlHandle() {
    return handle != nullptr ? handle->glHandle : 0;
}


GPUBufferPool::Buffer GPUBufferPool::allocate(GLuint size) {
    std::unique_lock<std::mutex> lock(poolLock);

    if (!pool.empty()) {
        // first try to find handle with
        for (auto it = pool.begin(); it != pool.end(); it++) {
            if ((*it)->size == size && (*it)->ownsLock.try_lock()) {
                Buffer buffer(std::move(*it), size);
                pool.erase(it);
                return buffer;
            }
        }

        // then try to find any buffer
        for (auto it = pool.begin(); it != pool.end(); it++) {
            if ((*it)->ownsLock.try_lock()) {
                Buffer buffer(std::move(*it), size);
                pool.erase(it);
                return buffer;
            }
        }
    }

    // if all failed - create a new one
    Buffer buffer(std::make_shared<BufferBase>(this), size);
    buffer.handle->ownsLock.lock();
    return buffer;
}

void GPUBufferPool::_release(std::shared_ptr<BufferBase> buffer) {
    std::unique_lock<std::mutex> lock(poolLock);
    pool.emplace_back(buffer);
}