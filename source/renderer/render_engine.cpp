#include <iostream>

#include "common/simple-profiler.h"
#include "render_engine.h"


VoxelRenderEngine::VoxelRenderEngine(std::shared_ptr<VoxelEngine> voxelEngine, std::shared_ptr<ChunkSource> chunkSource, std::shared_ptr<Camera> camera) :
        voxelEngine(std::move(voxelEngine)), chunkSource(std::move(chunkSource)), camera(std::move(camera)) {
    int maxBufferTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &maxBufferTextureSize);
    int bufferSize = std::min(maxBufferTextureSize, VoxelChunk::DEFAULT_CHUNK_BUFFER_SIZE * MAX_RENDER_CHUNK_INSTANCES);

    chunkBuffer = new gl::BufferTexture(bufferSize * sizeof(unsigned int), nullptr);
    chunkBufferSize = bufferSize / VoxelChunk::DEFAULT_CHUNK_BUFFER_SIZE;

    std::cout << "initialized voxel render engine, max_render_chunks = " << chunkBufferSize << " \n";
}

VoxelEngine* VoxelRenderEngine::getVoxelEngine() {
    return voxelEngine.get();
}

RenderChunk* VoxelRenderEngine::getNewRenderChunk(int reuseVisibilityLevel) {
    if (!renderChunkPool.empty()) {
        RenderChunk* result = *renderChunkPool.begin();
        renderChunkPool.pop_front();
        return result;
    } else {
        if (totalRenderChunkInstances < chunkBufferSize) {
            totalRenderChunkInstances++;
            RenderChunk* renderChunk = new RenderChunk(this);

            bool bufferOffsetFound = false;
            for (int i = 0; i < chunkBufferSize; i++) {
                if (!chunkBufferUsage[i]) {
                    chunkBufferUsage[i] = true;
                    renderChunk->setChunkBufferOffset(i * VoxelChunk::DEFAULT_CHUNK_BUFFER_SIZE);
                    bufferOffsetFound = true;
                    break;
                }
            }
            if (!bufferOffsetFound) {
                std::cout << "failed to find buffer offset!\n";
            }

            return renderChunk;
        } else {
            for (int reuseLevel = RenderChunk::VISIBILITY_LEVEL_NOT_VISIBLE; reuseLevel <= reuseVisibilityLevel; reuseLevel++) {
                for (auto it = renderChunkMap.begin(); it != renderChunkMap.end(); it++) {
                    if (it->second->visibilityLevel == reuseLevel) {
                        RenderChunk *renderChunk = it->second;
                        renderChunkMap.erase(it);
                        return renderChunk;
                    }
                }
            }
            return nullptr;
        }
    }

}

GLuint VoxelRenderEngine::getChunkBufferHandle() {
    return chunkBuffer->buffer_handle;
}

void VoxelRenderEngine::_queueRenderChunkUpdate(RenderChunk* renderChunk) {
    if (renderChunk->visibilityLevel == RenderChunk::VISIBILITY_LEVEL_NOT_VISIBLE || !renderChunk->hasAnyQueuedUpdates()) {
        return;
    }
    queuedRenderChunkUpdatesMutex.lock();
    queuedRenderChunkUpdates.insert(renderChunk);
    queuedRenderChunkUpdatesMutex.unlock();
}

void VoxelRenderEngine::runQueuedRenderChunkUpdates(int maxUpdateCount) {
    getVoxelEngine()->runOnGpuWorkerThread([=] () -> void {
        // move all queued chunks to local variable
        queuedRenderChunkUpdatesMutex.lock();
        std::unordered_set<RenderChunk*> chunksToUpdate = std::move(queuedRenderChunkUpdates);
        queuedRenderChunkUpdatesMutex.unlock();

        // total executed updates count
        int updateCount = 0;

        // run updates for visibility level: if able to update - run update, else - add back to queue
        std::function<void(int)> runUpdatesForVisibilityLevel = [&] (int level) -> void {
            for (RenderChunk* renderChunk : chunksToUpdate) {
                if (renderChunk->visibilityLevel == level) {
                    if (maxUpdateCount > 0 && updateCount >= maxUpdateCount) {
                        queuedRenderChunkUpdatesMutex.lock();
                        queuedRenderChunkUpdates.insert(renderChunk);
                        queuedRenderChunkUpdatesMutex.unlock();
                    } else {
                        updateCount += renderChunk->runAllUpdates();
                    }
                }
            }
        };

        // do for every visible chunk
        runUpdatesForVisibilityLevel(RenderChunk::VISIBILITY_LEVEL_VISIBLE);
        // then do for every near view chunk
        runUpdatesForVisibilityLevel(RenderChunk::VISIBILITY_LEVEL_NEAR_VIEW);

        // if any updated - swap buffers
        if (updateCount > 0) {
            // std::cout << "executed " << updateCount << " / " << maxUpdateCount << " render updates\n";
            getVoxelEngine()->swapGpuWorkerBuffers();
        }
    });
}

void VoxelRenderEngine::updateVisibleChunks() {
    PROFILER_BEGIN(VoxelRenderEngine_updateVisibleChunks);

    std::unordered_map<ChunkPos, int> visibilityUpdatesMap;
    camera->addAllVisiblePositions(visibilityUpdatesMap);
    std::list<std::pair<ChunkPos, int>> newVisibleChunks;
    for (auto const &posAndChunk : renderChunkMap) {
        posAndChunk.second->visibilityLevel = RenderChunk::VISIBILITY_LEVEL_NOT_VISIBLE;
    }

    for (auto const &posAndLevel : visibilityUpdatesMap) {
        auto it = renderChunkMap.find(posAndLevel.first);
        if (it != renderChunkMap.end()) {
            it->second->visibilityLevel = posAndLevel.second;
            _queueRenderChunkUpdate(it->second);
        } else {
            newVisibleChunks.emplace_back(posAndLevel);
        }
    }

    for (auto const &posAndLevel : newVisibleChunks) {
        if (posAndLevel.second > RenderChunk::VISIBILITY_LEVEL_NOT_VISIBLE) {
            VoxelChunk* chunk = chunkSource->getChunkAt(posAndLevel.first);
            if (chunk != nullptr && chunk->isAvailableForRender()) {
                RenderChunk* renderChunk = getNewRenderChunk(posAndLevel.second - 1);
                if (renderChunk != nullptr) {
                    renderChunkMap[posAndLevel.first] = renderChunk;
                    renderChunk->setPos(posAndLevel.first.x, posAndLevel.first.y, posAndLevel.first.z);
                    renderChunk->visibilityLevel = posAndLevel.second;
                    chunk->attachRenderChunk(renderChunk);

                    // update new chunk only if it is visible
                    if (renderChunk->visibilityLevel == RenderChunk::VISIBILITY_LEVEL_VISIBLE) {
                        _queueRenderChunkUpdate(renderChunk);
                    }
                }
            }
        }
    }

    /*
    int visible_count = 0;
    int near_visible_count = 0;
    int not_visible_count = 0;
    int pooled_count = renderChunkPool.size();

    for (auto const& posAndChunk : renderChunkMap) {
        if (posAndChunk.second->visibilityLevel == RenderChunk::VISIBILITY_LEVEL_VISIBLE) {
            visible_count++;
        }
        if (posAndChunk.second->visibilityLevel == RenderChunk::VISIBILITY_LEVEL_NEAR_VIEW) {
            near_visible_count++;
        }
        if (posAndChunk.second->visibilityLevel == RenderChunk::VISIBILITY_LEVEL_NOT_VISIBLE) {
            not_visible_count++;
        }
    }

    std::cout << "visible=" << visible_count << " near_visible=" << near_visible_count << " not_visible=" << not_visible_count << " pooled=" << pooled_count << "\n";
    */
}

void VoxelRenderEngine::prepareForRender(gl::Shader& shader) {
    PROFILER_BEGIN(VoxelRenderEngine_prepareForRender);

    int minX = 0x7FFFFFFF, minY = 0x7FFFFFFF, minZ = 0x7FFFFFFF;
    int maxX = -0x7FFFFFFF, maxY = -0x7FFFFFFF, maxZ = -0x7FFFFFFF;

    int chunksInView = 0;
    for (auto const &posAndChunk : renderChunkMap) {
        if (posAndChunk.second->visibilityLevel == RenderChunk::VISIBILITY_LEVEL_VISIBLE) {
            ChunkPos pos = posAndChunk.first;
            if (minX > pos.x) {
                minX = pos.x;
            }
            if (minY > pos.y) {
                minY = pos.y;
            }
            if (minZ > pos.z) {
                minZ = pos.z;
            }
            if (maxX < pos.x) {
                maxX = pos.x;
            }
            if (maxY < pos.y) {
                maxY = pos.y;
            }
            if (maxZ < pos.z) {
                maxZ = pos.z;
            }
            chunksInView++;
        }
    }

    if (chunksInView > 0) {
        int count[3] = {maxX - minX + 1, maxY - minY + 1, maxZ - minZ + 1};
        while (count[0] * count[1] * count[2] > chunkBufferSize) {
            // while visible chunk count is too big subtract 1 from largest axis count
            if (count[0] > count[1]) {
                if (count[2] > count[0]) {
                    count[2]--;
                } else {
                    count[0]--;
                }
            } else {
                if (count[2] > count[1]) {
                    count[2]--;
                } else {
                    count[1]--;
                }
            }
        }

        int offset[3] = {minX + (maxX - minX - count[0]) / 2, minY + (maxY - minY - count[1]) / 2,
                         minZ + (maxZ - minZ - count[2]) / 2};

        GLint chunkTextureOffsets[MAX_RENDER_CHUNK_INSTANCES] { 128 };
        for (auto const& posAndChunk : renderChunkMap) {
            if (posAndChunk.second->visibilityLevel == RenderChunk::VISIBILITY_LEVEL_VISIBLE) {
                ChunkPos pos = posAndChunk.first;
                if (pos.x >= offset[0] && pos.y >= offset[1] && pos.z >= offset[2] &&
                    pos.x < offset[0] + count[0] && pos.y < offset[1] + count[1] && pos.z < offset[2] + count[2]) {
                    chunkTextureOffsets[(pos.x - offset[0]) + ((pos.z - offset[2]) + (pos.y - offset[1]) * count[2]) *
                                                              count[0]] = posAndChunk.second->chunkBufferOffset;
                }
            }
        }

        // std::cout << " total=" << renderChunkMap.size() << " visible=" << chunksInView << " rendered=" << visible_count << "/" << (count[0] * count[1] * count[2]) << "\n";

        glActiveTexture(GL_TEXTURE0);
        chunkBuffer->bind();

        shader.use();

        UNIFORM_HANDLE(chunk_offset_uniform, shader, "CHUNK_OFFSET");
        UNIFORM_HANDLE(chunk_count_uniform, shader, "CHUNK_COUNT");
        UNIFORM_HANDLE(chunk_buffer_uniform, shader, "CHUNK_BUFFER");
        UNIFORM_HANDLE(chunk_buffer_offsets_uniform, shader, "CHUNK_DATA_OFFSETS_IN_BUFFER");

        glUniform3i(chunk_offset_uniform, offset[0], offset[1], offset[2]);
        glUniform3i(chunk_count_uniform, count[0], count[1], count[2]);
        glUniform1ui(chunk_buffer_uniform, 1);
        glUniform1iv(chunk_buffer_offsets_uniform, MAX_RENDER_CHUNK_INSTANCES, chunkTextureOffsets);
    } else {
        // no chunks in view
        glUniform3i(shader.getUniform("CHUNK_COUNT"), 0, 0, 0);
    }

}

