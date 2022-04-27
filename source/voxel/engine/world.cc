#include "world.h"
#include "voxel/engine/world/chunk_provider.h"
#include "voxel/engine/world/chunk_storage.h"
#include "voxel/engine/world/chunk_source.h"


namespace voxel {

World::World(Unique<ChunkProvider> chunk_provider,
             Unique<ChunkStorage> chunk_storage,
             const threading::TickingThread::Scheduler& tick_scheduler,
             ChunkSource::Settings chunk_source_settings) :
             m_chunk_source(CreateShared<ChunkSource>(std::move(chunk_provider), std::move(chunk_storage), chunk_source_settings)),
             m_ticking_thread(tick_scheduler, std::bind(&World::onTick, this)) {
}

World::~World() {
    setTicking(false);
    m_ticking_thread.join();
}

void World::setTicking(bool ticking) {
    m_ticking_thread.setRunning(ticking);
}

void World::joinTickingThread() {
    m_ticking_thread.join();
}

const Shared<ChunkSource>& World::getChunkSource() const {
    return m_chunk_source;
}

void World::onTick() {
    m_chunk_source->onTick();
}

} // voxel