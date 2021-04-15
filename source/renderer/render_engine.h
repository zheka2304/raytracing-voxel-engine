
#ifndef VOXEL_ENGINE_RENDER_ENGINE_H
#define VOXEL_ENGINE_RENDER_ENGINE_H

#include <mutex>
#include <memory>
#include <list>
#include <unordered_map>
#include <unordered_set>

#include "engine.h"
#include "engine/chunk_source.h"
#include "render_chunk.h"
#include "camera.h"
#include "util.h"


class VoxelRenderEngine {
private:
    static const int MAX_RENDER_CHUNK_INSTANCES = 64;

    std::shared_ptr<VoxelEngine> voxelEngine;

    std::mutex mutex;

    //
    int totalRenderChunkInstances = 0;
    // pool is used to reuse old render chunks objects instead of destroying them
    std::list<RenderChunk*> renderChunkPool;

    // chunk by position map
    std::unordered_map<ChunkPos, RenderChunk*> renderChunkMap;

    // chunk source for world access
    std::shared_ptr<ChunkSource> chunkSource;

    // camera, that calculates visible chunks
    std::shared_ptr<Camera> camera;

    // all chunks are stored in single buffer because sampler uniform arrays are not always supported
    gl::Buffer* chunkBuffer;

    // maximum chunks in buffer
    int chunkBufferSize;
    bool chunkBufferUsage[MAX_RENDER_CHUNK_INSTANCES] = { false };

    // update queue for render chunks
    std::mutex queuedRenderChunkUpdatesMutex;
    std::unordered_set<RenderChunk*> queuedRenderChunkUpdates;

private:
    struct BufferOffsets {
        int offsets[64];
    };
    gl::ComputeShaderUniform<BufferOffsets> u_BufferOffsets;

    struct RenderRegion {
        GLSL_BUFFER_ALIGN Vec3i offset;
        GLSL_BUFFER_ALIGN Vec3i count;
    };
    gl::ComputeShaderUniform<RenderRegion> u_RenderRegion;

    struct AmbientData {
        GLSL_BUFFER_ALIGN float directLightColor[4];
        GLSL_BUFFER_ALIGN float directLightRay[3];
        GLSL_BUFFER_ALIGN float ambientLightColor[4];
    };
    gl::ComputeShaderUniform<AmbientData> u_AmbientData;

    gl::ComputeShaderUniform<Camera::UniformData> u_CameraData;


    void _queueRenderChunkUpdate(RenderChunk* renderChunk);

public:
    struct ScreenParameters {
        int width, height;
    };

    ScreenParameters screenParameters;

    gl::Texture o_ColorTexture;
    gl::Texture o_LightTexture;
    gl::Texture o_DepthTexture;

    VoxelRenderEngine(std::shared_ptr<VoxelEngine> voxelEngine, std::shared_ptr<ChunkSource> chunkSource, std::shared_ptr<Camera> camera, ScreenParameters screenParams);

    RenderChunk* getNewRenderChunk(int maxLevelToReuse = RenderChunk::VISIBILITY_LEVEL_NOT_VISIBLE);

    // refresh visible chunks from camera, update all visibility levels, pool unused chunks
    // for all visible chunks, queues render updates if required
    void updateVisibleChunks();

    // runs all queued updates for existing chunks
    // async, adds task on gpu worker thread
    void runQueuedRenderChunkUpdates(int maxUpdateCount);

    // for all visible chunks, map all textures, pass to uniforms
    // passed shader should contain following uniforms:
    // - CHUNK_OFFSET - ivec3 offset of chunk region to be rendered
    // - CHUNK_COUNT - ivec3 of chunk count on each axis, total count should not be greater then 32
    // - CHUNK_BUFFERS - array of usamplerBuffer with size of 32
    void render();

    GLuint getChunkBufferHandle();

    VoxelEngine* getVoxelEngine();

    ~VoxelRenderEngine();
};


#endif //VOXEL_ENGINE_RENDER_ENGINE_H
