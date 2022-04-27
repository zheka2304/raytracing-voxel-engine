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
    i32 chunk_updates_per_tick = 8;

    // Maximum count of chunks, fetched each tick
    i32 chunk_fetches_per_tick = 4096;

    // distance from previous offset to rebuild chunk buffer map
    i32 buffer_offset_update_distance = 4;

    // loading level for camera region
    i32 chunk_loading_level = 40;
};

class WorldRenderer : public ChunkSourceListener {
    Shared<ChunkSource> m_chunk_source;
    Unique<render::ChunkBuffer> m_chunk_buffer;

    math::Vec3f m_camera_position = math::Vec3f(0.0);
    math::Vec3i m_chunk_map_offset_position = math::Vec3i(0);
    std::atomic<bool> m_rebuild_chunk_map = false;
    Shared<ChunkSource::LoadingRegion> m_camera_loading_region;

    WorldRendererSettings m_settings;
    render::FetchedChunksList m_fetched_chunks_list;
    std::atomic<bool> m_request_fetched_chunks = true;
    i64 m_chunk_fetch_priority = 0;

    threading::UniqueBlockingQueue<ChunkRef> m_chunk_updates;

public:
    WorldRenderer(Shared<ChunkSource> chunk_source, Unique<render::ChunkBuffer> chunk_buffer, WorldRendererSettings settings);
    WorldRenderer(const WorldRenderer&) = delete;
    WorldRenderer(WorldRenderer&&) = delete;
    ~WorldRenderer();

    void addChunkToUpdateQueue(ChunkRef chunk_ref);
    void setCameraPosition(math::Vec3f camera_position);
    void render(render::RenderContext& render_context);
    void onTick();

private:
    void onChunkSourceTick(ChunkSource &chunk_source) override;
    void onChunkUpdated(ChunkSource &chunk_source, ChunkRef chunk_ref) override;

    void fetchRequestedChunks();
    void runChunkUpdates();
};

} // voxel

#endif //VOXEL_ENGINE_WORLD_RENDERER_H
