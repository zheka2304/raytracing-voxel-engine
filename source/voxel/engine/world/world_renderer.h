#ifndef VOXEL_ENGINE_WORLD_RENDERER_H
#define VOXEL_ENGINE_WORLD_RENDERER_H

#include <atomic>
#include "voxel/common/base.h"
#include "voxel/common/threading.h"
#include "voxel/engine/render/render_context.h"
#include "voxel/engine/render/chunk_buffer.h"
#include "voxel/engine/world/chunk_source.h"


namespace voxel {

struct WorldRendererSettings {
    // Maximum count of chunks to be uploaded or removed each frame
    i32 chunk_updates_per_frame = 16;

    // Amount of rays, requiring chunk fetch for it to be fetched = amount of visible chunk pixels
    i32 fetch_ray_count_threshold = 1;

    // Maximum count of chunks, fetched each tick
    i32 chunk_fetches_per_tick = 512;
};

class WorldRenderer : public ChunkSourceListener {
    Shared<ChunkSource> m_chunk_source;
    Shared<render::ChunkBuffer> m_chunk_buffer;

    WorldRendererSettings m_settings;
    render::FetchedChunksList m_fetched_chunks_list;
    std::atomic<bool> m_request_fetched_chunks = true;

    threading::UniqueBlockingQueue<Shared<Chunk>> m_chunk_updates;

public:
    WorldRenderer(Shared<ChunkSource> chunk_source, Shared<render::ChunkBuffer> chunk_buffer, WorldRendererSettings settings);
    WorldRenderer(const WorldRenderer&) = delete;
    WorldRenderer(WorldRenderer&&) = delete;
    ~WorldRenderer();

    void addChunkToUpdateQueue(Shared<Chunk> chunk);
    void render(render::RenderContext& render_context);
    void onTick();

private:
    void onChunkSourceTick(ChunkSource &chunk_source) override;
    void onChunkUpdated(ChunkSource &chunk_source, Shared<Chunk> chunk) override;

    void fetchRequestedChunks();
};

} // voxel

#endif //VOXEL_ENGINE_WORLD_RENDERER_H
