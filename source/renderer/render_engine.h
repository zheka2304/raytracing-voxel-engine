
#ifndef VOXEL_ENGINE_RENDER_ENGINE_H
#define VOXEL_ENGINE_RENDER_ENGINE_H

#include <mutex>
#include <memory>
#include <list>
#include <unordered_map>

#include "engine/chunk_source.h"
#include "render_chunk.h"
#include "camera.h"
#include "util.h"


class VoxelRenderEngine {
    static const int MAX_RENDER_CHUNK_INSTANCES = 64;

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
    gl::BufferTexture* chunkBuffer;

    // maximum chunks in buffer
    int chunkBufferSize;
    bool chunkBufferUsage[MAX_RENDER_CHUNK_INSTANCES] = { false };

public:
    VoxelRenderEngine(std::shared_ptr<ChunkSource> chunkSource, std::shared_ptr<Camera> camera);

    RenderChunk* getNewRenderChunk(int maxLevelToReuse = RenderChunk::VISIBILITY_LEVEL_NOT_VISIBLE);

    // refresh visible chunks from camera, update all visibility levels, pool unused chunks
    // for all visible chunks, run queued updates
    void updateVisibleChunks();

    // for all visible chunks, map all textures, pass to uniforms
    // passed shader should contain following uniforms:
    // - CHUNK_OFFSET - ivec3 offset of chunk region to be rendered
    // - CHUNK_COUNT - ivec3 of chunk count on each axis, total count should not be greater then 32
    // - CHUNK_BUFFERS - array of usamplerBuffer with size of 32
    void prepareForRender(gl::Shader& shader);

    GLuint getChunkBufferHandle();
};


#endif //VOXEL_ENGINE_RENDER_ENGINE_H
