#ifndef VOXEL_ENGINE_WORLD_H
#define VOXEL_ENGINE_WORLD_H

#include "voxel/common/threading.h"
#include "voxel/engine/world/chunk.h"
#include "voxel/engine/world/chunk_source.h"

namespace voxel {

class World {
    Shared<ChunkSource> m_chunk_source;
    Shared<threading::TaskExecutor> m_background_executor;
    threading::TickingThread m_ticking_thread;

public:
    World(Shared<ChunkProvider> chunk_provider,
          Shared<ChunkStorage> chunk_storage,
          Shared<threading::TaskExecutor> background_executor,
          const threading::TickingThread::Scheduler& tick_scheduler,
          ChunkSource::Settings chunk_source_settings);
    World(const World&) = delete;
    World(World&&) = delete;
    ~World();

    void setTicking(bool ticking);
    const Shared<ChunkSource>& getChunkSource() const;
    const Shared<threading::TaskExecutor>& getBackgroundExecutor() const;

protected:
    void onTick();
};

} // voxel

#endif //VOXEL_ENGINE_WORLD_H
