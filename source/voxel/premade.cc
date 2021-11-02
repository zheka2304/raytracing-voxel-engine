#include "premade.h"

#include <fstream>
#include "voxel/engine/world/chunk_provider.h"
#include "voxel/engine/world/chunk_storage.h"
#include "voxel/engine/file/vox_file_format.h"


namespace voxel {

Shared<VoxelModel> loadVoxelModelFromVoxFile(const std::string& filename) {
    format::VoxFileFormat file_format;
    std::ifstream istream(filename, std::ifstream::binary);
    auto models = file_format.read(istream);
    istream.close();
    return !models.empty() ? models[0] : Shared<VoxelModel>();
}


class SingleChunkModelChunkProvider : public ChunkProvider {
    Shared<VoxelModel> m_model;
    Voxel m_ground_material;

public:
    SingleChunkModelChunkProvider(Shared<VoxelModel> model, Voxel ground_mat) : m_model(model), m_ground_material(ground_mat) {
    }

    bool canFetchChunk(ChunkSource& chunk_source, ChunkPosition position) override {
        return position.y == 0;
    }

    Shared<Chunk> createChunk(ChunkSource &chunk_source, ChunkPosition position) override {
        return CreateShared<Chunk>(position);
    }

    bool buildChunk(ChunkSource &chunk_source, Shared<Chunk> chunk) override {
        VoxelModel& model = *m_model.get();
        auto size = model.getSize();

        i32 max_size = std::max(size.x, std::max(size.y, size.z));
        u8 scale = 1;
        while (max_size >>= 1) scale++;

        if (chunk->getPosition().x == 0 && chunk->getPosition().z == 0) {
            for (unsigned int x = 0; x < size.x; x++) {
                for (unsigned int y = 0; y < size.y; y++) {
                    for (unsigned int z = 0; z < size.z; z++) {
                        Voxel voxel = model.getVoxel(x, y, z);
                        voxel.material = 0;
                        if (voxel.color != 0) {
                            voxel.color |= (31 << 25);
                            chunk->setVoxel({scale, x, y, z}, voxel);
                        }
                    }
                }
            }
        }

        u32 chunk_size = 1u << scale;
        for (unsigned int x = 0; x < chunk_size; x++) {
            for (unsigned int z = 0; z < chunk_size; z++) {
                chunk->setVoxel({7, x, 0, z}, m_ground_material);
            }
        }

        return true;
    }
};

Unique<World> createWorldFromModel(Shared<VoxelModel> model, Voxel ground_voxel) {
    //

    Shared<ChunkProvider> chunk_provider = CreateShared<SingleChunkModelChunkProvider>(model, ground_voxel);
    Shared<ChunkStorage> chunk_storage = CreateShared<ChunkStorage>();
    Unique<World> world = CreateUnique<World>(
            chunk_provider, chunk_storage,
            threading::TickingThread::TicksPerSecond(20),
            ChunkSource::Settings());
    return world;
}

} //