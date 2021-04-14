

#include <iostream>
#include "gpu_cache.h"

std::atomic<GLuint> GPUBufferPool::lastContentUuid = 1;

GPUBufferPool::BufferBase::BufferBase(GPUBufferPool* pool) : pool(pool) {
    contentUuid = GPUBufferPool::lastContentUuid++;
    glGenBuffers(1, &glHandle);
}

void GPUBufferPool::BufferBase::resize(GLuint newSize) {
    pool->totalMemoryUsed += (newSize - size);
    size = newSize;
}

GPUBufferPool::BufferBase::~BufferBase() {
    resize(0);
    glDeleteBuffers(1, &glHandle);
}


GPUBufferPool::Buffer::Buffer(std::shared_ptr<BufferBase> handle, GLuint size) : handle(handle) {
    handle->resize(size);
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

GLuint GPUBufferPool::Buffer::getStoredContentUuid() {
    return handle != nullptr ? handle->contentUuid : 0;
}

GLuint GPUBufferPool::Buffer::getLocalContentUuid() {
    return contentUuid;
}

GLuint GPUBufferPool::Buffer::getGlHandle() {
    return handle != nullptr ? handle->glHandle : 0;
}


GPUBufferPool::GPUBufferPool(GLuint poolSize) : maxMemoryUsed(poolSize) {

}

GPUBufferPool::Buffer GPUBufferPool::allocate(GLuint size) {
    std::unique_lock<std::mutex> lock(poolLock);

    if (!pool.empty() && totalMemoryUsed + size >= maxMemoryUsed) {
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