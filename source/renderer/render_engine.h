
#ifndef VOXEL_ENGINE_RENDER_ENGINE_H
#define VOXEL_ENGINE_RENDER_ENGINE_H

#include <mutex>
#include <memory>
#include <list>
#include <unordered_map>
#include <unordered_set>

#include "common/chunk_pos.h"
#include "engine.h"
#include "engine/chunk_source.h"
#include "render_chunk.h"
#include "camera.h"
#include "util.h"


class VoxelRenderEngine;


class VoxelLightingEngine {
private:
    int maxLightChunks;
    gl::Buffer* lightBuffer;

public:
    static const int WORK_GROUP_SIZE = 1024;
    static const int LIGHT_VOXEL_SIZE = 4;
    static const int LIGHT_REFLECTION_COUNT = 4;
    static const int LIGHT_CHUNK_SIZE = ChunkPos::CHUNK_SIZE / LIGHT_VOXEL_SIZE;
    static const int LIGHT_CHUNK_VOLUME = LIGHT_CHUNK_SIZE * LIGHT_CHUNK_SIZE * LIGHT_CHUNK_SIZE;

    static const int SHADER_BINDING_LIGHT_PASS_BUFFER = 12;
    static const int SHADER_BINDING_LIGHT_PASS_OFFSETS = 13;
    static const int SHADER_BINDING_LIGHT_PASS_JOBS = 14;


    struct LightBufferOffsets {
        GLSL_BUFFER_ALIGN16 Vec3i chunkBufferRegionOffset;
        GLSL_BUFFER_ALIGN16 Vec3i chunkBufferRegionSize;
        GLSL_BUFFER_ALIGN4 int offsets[64];
    };
    gl::ComputeShaderUniform<LightBufferOffsets, SHADER_BINDING_LIGHT_PASS_OFFSETS> u_LightBufferOffsets;

    struct ProcessLightJob {
        GLSL_BUFFER_ALIGN4 int level;
        GLSL_BUFFER_ALIGN16 Vec3i chunkPositionOffset;
        GLSL_BUFFER_ALIGN4 int chunkBufferOffset;
        GLSL_BUFFER_ALIGN4 int chunkBufferRegionSize;
        GLSL_BUFFER_ALIGN4 int chunkBufferStride;
    };

    struct ProcessLightJobList {
        ProcessLightJob jobs[64];
    };
    gl::ComputeShaderUniform<ProcessLightJobList, SHADER_BINDING_LIGHT_PASS_JOBS> u_ProcessLightJobList;


    explicit VoxelLightingEngine(int maxLightChunks);
    ~VoxelLightingEngine() noexcept;

public:
    GLuint getBufferHandle();
    Vec3i prepareLightPass(VoxelRenderEngine& engine, Vec3i renderRegionOffset, Vec3i renderRegionSize);
};



class VoxelRenderEngine {
private:
    static const int MAX_RENDER_CHUNK_INSTANCES = 64;
    // amount of floats per pixel of pre-raytrace buffer
    static const int PRE_RAYTRACE_DATA_PER_PIXEL = 4;

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

    // buffer for pre-raytracing stage, contains downscaled data for ray offsets
    gl::Buffer* preRenderBuffer;

    VoxelLightingEngine lightingEngine = VoxelLightingEngine(MAX_RENDER_CHUNK_INSTANCES);


    // maximum chunks in buffer
    int chunkBufferSize;
    bool chunkBufferUsage[MAX_RENDER_CHUNK_INSTANCES] = { false };

    // update queue for render chunks
    std::mutex queuedRenderChunkUpdatesMutex;
    std::unordered_set<RenderChunk*> queuedRenderChunkUpdates;

private:
    static const int SHADER_BINDING_OUT_COLOR_TEXTURE = 0;
    static const int SHADER_BINDING_OUT_LIGHT_TEXTURE = 1;
    static const int SHADER_BINDING_OUT_DEPTH_TEXTURE = 2;

    static const int SHADER_BINDING_CHUNK_VOXEL_BUFFER = 3;
    static const int SHADER_BINDING_CHUNK_BUFFER_OFFSETS = 4;
    static const int SHADER_BINDING_RENDER_REGION = 5;
    static const int SHADER_BINDING_AMBIENT_DATA = 6;
    static const int SHADER_BINDING_CAMERA_DATA = 7;
    static const int SHADER_BINDING_PRE_PASS_BUFFER = 8;
    static const int SHADER_BINDING_PRE_PASS_DATA = 9;

    struct BufferOffsets {
        int offsets[64];
    };
    gl::ComputeShaderUniform<BufferOffsets, SHADER_BINDING_CHUNK_BUFFER_OFFSETS> u_BufferOffsets;

    struct RenderRegion {
        GLSL_BUFFER_ALIGN Vec3i offset;
        GLSL_BUFFER_ALIGN Vec3i count;
    };
    gl::ComputeShaderUniform<RenderRegion, SHADER_BINDING_RENDER_REGION> u_RenderRegion;

    struct AmbientData {
        GLSL_BUFFER_ALIGN float directLightColor[4];
        GLSL_BUFFER_ALIGN float directLightRay[3];
        GLSL_BUFFER_ALIGN float ambientLightColor[4];
    };
    gl::ComputeShaderUniform<AmbientData, SHADER_BINDING_AMBIENT_DATA> u_AmbientData;

    gl::ComputeShaderUniform<Camera::UniformData, SHADER_BINDING_CAMERA_DATA> u_CameraData;

    struct PreRenderPassData {
        int strideBit;
        GLSL_BUFFER_ALIGN8 int bufferSize[2];
        GLSL_BUFFER_ALIGN4 float dis1;
        GLSL_BUFFER_ALIGN4 float dis2;
    };
    gl::ComputeShaderUniform<PreRenderPassData, SHADER_BINDING_PRE_PASS_DATA> u_PreRenderPassData;

    void _queueRenderChunkUpdate(RenderChunk* renderChunk);

public:
    struct ScreenParameters {
        int width, height;
        int prerenderStrideBit = 2;
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
