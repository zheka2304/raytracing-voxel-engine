#ifndef VOXEL_ENGINE_WORLD_H
#define VOXEL_ENGINE_WORLD_H

#include "voxel/common/threading.h"
#include "voxel/engine/world/chunk.h"
#include "voxel/engine/world/chunk_source.h"

namespace voxel {

class World {
    ChunkSource m_chunk_source;
    threading::TickingThread m_ticking_thread;
    Shared<threading::TaskExecutor> m_background_executor;

public:
    World(const threading::TickingThread::Scheduler& tick_scheduler,
          Shared<ChunkProvider> chunk_provider,
          Shared<ChunkStorage> chunk_storage,
          Shared<threading::TaskExecutor> background_executor);
    World(const World&) = delete;
    World(World&&) = delete;
    ~World();

    ChunkSource& getChunkSource();
    threading::TaskExecutor& getBackgroundExecutor();

protected:
    void onTick();
};

} // voxel

#endif //VOXEL_ENGINE_WORLD_H
