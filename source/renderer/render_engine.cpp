#include <iostream>

#include "common/simple-profiler.h"
#include "render_engine.h"


VoxelRenderEngine::VoxelRenderEngine(std::shared_ptr<VoxelEngine> voxelEngine, std::shared_ptr<ChunkSource> chunkSource, std::shared_ptr<Camera> camera, ScreenParameters screenParams) :
        voxelEngine(std::move(voxelEngine)), chunkSource(std::move(chunkSource)), camera(std::move(camera)), screenParameters(screenParams),
        o_ColorTexture(screenParams.width, screenParams.height, GL_RGBA32F, GL_RGBA, GL_FLOAT),
        o_DepthTexture(screenParams.width, screenParams.height, GL_RGBA32F, GL_RGBA, GL_FLOAT),
        o_LightTexture(screenParams.width, screenParams.height, GL_RGBA32F, GL_RGBA, GL_FLOAT) {
    int maxBufferTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &maxBufferTextureSize);
    int bufferSize = std::min(maxBufferTextureSize, VoxelChunk::DEFAULT_CHUNK_BUFFER_SIZE * MAX_RENDER_CHUNK_INSTANCES);

    chunkBuffer = new gl::Buffer();
    chunkBuffer->setData(bufferSize * sizeof(baked_voxel_t), nullptr, GL_SHADER_STORAGE_BUFFER, GL_DYNAMIC_DRAW);
    chunkBufferSize = bufferSize / VoxelChunk::DEFAULT_CHUNK_BUFFER_SIZE;

    preRenderBuffer = new gl::Buffer();
    preRenderBuffer->setData((screenParams.width >> screenParams.prerenderStrideBit) * (screenParams.height >> screenParams.prerenderStrideBit) * sizeof(float) * PRE_RAYTRACE_DATA_PER_PIXEL, nullptr);

    std::cout << "initialized voxel render engine, max_render_chunks = " << chunkBufferSize << " \n";
}

VoxelRenderEngine::~VoxelRenderEngine() {
    delete(chunkBuffer);
    delete(preRenderBuffer);
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
    return chunkBuffer->handle;
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

void VoxelRenderEngine::render() {
    PROFILER_BEGIN(VoxelRenderEngine_prepareForRender);

    // initialize render passes and related variables
    int prerenderWidth = screenParameters.width >> screenParameters.prerenderStrideBit;
    int prerenderHeight = screenParameters.height >> screenParameters.prerenderStrideBit;

    struct RenderPass {
        gl::ComputeShader shader;
        int width, height;
        int workGroupSize;
    };

    static std::vector<RenderPass> renderPasses;
    if (renderPasses.empty()) {
        renderPasses.emplace_back(RenderPass({gl::ComputeShader("raytrace_screen_pass.compute", { "RENDER_STAGE_PRE_PASS" }), prerenderWidth, prerenderHeight, 24}));
        renderPasses.emplace_back(RenderPass({gl::ComputeShader("raytrace_buffer_pass.compute", { }), prerenderWidth, prerenderHeight, 24}));
        renderPasses.emplace_back(RenderPass({gl::ComputeShader("raytrace_screen_pass.compute", { "RENDER_STAGE_PASS_MAIN" }), screenParameters.width, screenParameters.height, 24}));
    }

    //

    static gl::ComputeShader lightPassInitial("raytrace_light_pass.compute", { "INITIAL_PASS" });
    static gl::ComputeShader lightPassIterative("raytrace_light_pass.compute", { });

    //

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


    Vec3i lightingJobCount(0, 0, 0);

    if (chunksInView > 0) {
        Vec3i count(maxX - minX + 1, maxY - minY + 1, maxZ - minZ + 1);
        while (count.x * count.y * count.z > chunkBufferSize) {
            // while visible chunk count is too big subtract 1 from largest axis count
            if (count.x > count.y) {
                if (count.z > count.x) {
                    count.z--;
                } else {
                    count.x--;
                }
            } else {
                if (count.z > count.y) {
                    count.z--;
                } else {
                    count.y--;
                }
            }
        }

        Vec3i offset(minX + (maxX - minX - count.x) / 2, minY + (maxY - minY - count.y) / 2, minZ + (maxZ - minZ - count.z) / 2);

        for (auto const& posAndChunk : renderChunkMap) {
            if (posAndChunk.second->visibilityLevel == RenderChunk::VISIBILITY_LEVEL_VISIBLE) {
                ChunkPos pos = posAndChunk.first;
                if (pos.x >= offset.x && pos.y >= offset.y && pos.z >= offset.z &&
                    pos.x < offset.x + count.x && pos.y < offset.y + count.y && pos.z < offset.z + count.z) {
                    u_BufferOffsets.data.offsets[(pos.x - offset.x) + ((pos.z - offset.z) + (pos.y - offset.y) * count.z) *
                                                              count.x] = posAndChunk.second->chunkBufferOffset;
                }
            }
        }

        // std::cout << " total=" << renderChunkMap.size() << " visible=" << chunksInView << " rendered=" << visible_count << "/" << (count[0] * count[1] * count[2]) << "\n";

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SHADER_BINDING_CHUNK_VOXEL_BUFFER, chunkBuffer->handle);
        u_BufferOffsets.updateAndBind();
        u_RenderRegion.data = {
                offset, count
        };
        u_RenderRegion.updateAndBind();

        lightingJobCount = lightingEngine.prepareLightPass(*this, offset, count);
    } else {
        // no chunks in view
        u_RenderRegion.data.count = Vec3i(0, 0, 0);
        u_RenderRegion.updateAndBind();
    }

    u_AmbientData.data = {
            {1, 1, 1, 1},
            {1, -0.7f, 1},
            {0, 0, 0.3f, 0.5f}
    };
    u_AmbientData.updateAndBind();

    camera->sendParametersToShader(u_CameraData.data);
    u_CameraData.updateAndBind();

    float lodDis1, lodDis2;
    camera->getLodDistances(lodDis1, lodDis2);
    u_PreRenderPassData.data = {
            screenParameters.prerenderStrideBit,
            { prerenderWidth, prerenderHeight },
            lodDis1, lodDis2
    };
    u_PreRenderPassData.updateAndBind();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SHADER_BINDING_PRE_PASS_BUFFER, preRenderBuffer->handle);

    glBindImageTexture(SHADER_BINDING_OUT_COLOR_TEXTURE, o_ColorTexture.handle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(SHADER_BINDING_OUT_LIGHT_TEXTURE, o_LightTexture.handle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindImageTexture(SHADER_BINDING_OUT_DEPTH_TEXTURE, o_DepthTexture.handle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // run light pass
    if (lightingJobCount.y > 0) {
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        lightPassInitial.use();
        glDispatchCompute(lightingJobCount.x, lightingJobCount.y, lightingJobCount.z);
    }

    // run render passes
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    for (auto const& pass : renderPasses) {
        pass.shader.use();
        glDispatchCompute((pass.width + (pass.workGroupSize - 1)) / pass.workGroupSize, (pass.height + (pass.workGroupSize - 1)) / pass.workGroupSize, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }
}




VoxelLightingEngine::VoxelLightingEngine(int maxLightChunks) : maxLightChunks(maxLightChunks) {
    lightBuffer = new gl::Buffer();
    lightBuffer->setData(maxLightChunks * LIGHT_CHUNK_VOLUME * (LIGHT_BUFFER_UNIT_COUNT * sizeof(GLuint)), nullptr);
}

VoxelLightingEngine::~VoxelLightingEngine() noexcept {
    delete(lightBuffer);
}


Vec3i VoxelLightingEngine::prepareLightPass(VoxelRenderEngine& engine, Vec3i renderRegionOffset, Vec3i renderRegionSize) {
    static int frame = 0;
    int level = frame++ % 2;

    u_LightBufferOffsets.data.chunkBufferRegionOffset = renderRegionOffset;
    u_LightBufferOffsets.data.chunkBufferRegionSize = renderRegionSize;

    int jobCount = 0;
    int maxWorkGroupsPerJob = 0;

    for (int x = 0; x < renderRegionSize.x; x++) {
        for (int y = 0; y < renderRegionSize.y; y++) {
            for (int z = 0; z < renderRegionSize.z; z++) {
                int offset = jobCount * (32 * 32 * 32);
                int size = 32 * 32 * 32;

                u_ProcessLightJobList.data.jobs[jobCount] = {
                    level,
                    Vec3i(x, y, z) + renderRegionOffset,
                    offset,
                    (32 * 32 * 32),
                    1
                };
                u_LightBufferOffsets.data.offsets[x + (z + y * renderRegionSize.z) * renderRegionSize.x] = offset;

                maxWorkGroupsPerJob = std::max(maxWorkGroupsPerJob, size / WORK_GROUP_SIZE);

                jobCount++;
                if (jobCount >= maxLightChunks) {
                    goto loopEnd;
                }
            }
        }
    }
    loopEnd:

    u_LightBufferOffsets.updateAndBind();
    u_ProcessLightJobList.updateAndBind();
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SHADER_BINDING_LIGHT_PASS_BUFFER, lightBuffer->handle);

    return Vec3i(maxWorkGroupsPerJob, jobCount, 1);
}

GLuint VoxelLightingEngine::getBufferHandle() {
    return lightBuffer->handle;
}