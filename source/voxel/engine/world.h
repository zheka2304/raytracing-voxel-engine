#ifndef VOXEL_ENGINE_WORLD_H
#define VOXEL_ENGINE_WORLD_H

#include "voxel/common/threading.h"
#include "voxel/engine/world/chunk.h"
#include "voxel/engine/world/chunk_source.h"


namespace voxel {

class World {
    Shared<ChunkSource> m_chunk_source;
    threading::TickingThread m_ticking_thread;

public:
    World(Unique<ChunkProvider> chunk_provider,
          Unique<ChunkStorage> chunk_storage,
          const threading::TickingThread::Scheduler& tick_scheduler,
          ChunkSource::Settings chunk_source_settings);
    World(const World&) = delete;
    World(World&&) = delete;
    ~World();

    void setTicking(bool ticking);
    void joinTickingThread();

    const Shared<ChunkSource>& getChunkSource() const;

protected:
    void onTick();
};

} // voxel

#endif //VOXEL_ENGINE_WORLD_H
