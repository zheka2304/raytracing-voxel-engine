#include "premade.h"

#include <fstream>
#include "voxel/engine/world/chunk_provider.h"
#include "voxel/engine/world/chunk_storage.h"
#include "voxel/engine/file/vox_file_format.h"


namespace voxel {

Unique<VoxelModel> loadVoxelModelFromVoxFile(const std::string& filename) {
    format::VoxFileFormat file_format;
    std::ifstream istream(filename, std::ifstream::binary);
    auto models = file_format.read(istream);
    istream.close();
    return !models.empty() ? CreateUnique<VoxelModel>(std::move(models[0])) : nullptr;
}


class SingleChunkModelChunkProvider : public ChunkProvider {
    Unique<VoxelModel> m_model;
    Voxel m_ground_material;

public:
    SingleChunkModelChunkProvider(Unique<VoxelModel> model, Voxel ground_mat) : m_model(std::move(model)), m_ground_material(ground_mat) {
    }

    bool canFetchChunk(ChunkSource& chunk_source, ChunkPosition position) override {
        return position.y == 0;
    }

    Unique<Chunk> createChunk(ChunkSource &chunk_source, ChunkPosition position) override {
        return CreateUnique<Chunk>(position);
    }

    bool buildChunk(ChunkSource &chunk_source, Chunk& chunk) override {
        VoxelModel& model = *m_model.get();
        auto size = model.getSize();

        i32 max_size = std::max(size.x, std::max(size.y, size.z));
        u8 scale = 1;
        while (max_size >>= 1) scale++;

        if (chunk.getPosition().x == 0 && chunk.getPosition().z == 0) {
            for (unsigned int x = 0; x < size.x; x++) {
                for (unsigned int y = 0; y < size.y; y++) {
                    for (unsigned int z = 0; z < size.z; z++) {
                        Voxel voxel = model.getVoxel(x, y, z);
                        voxel.material = 0;
                        if (voxel.color != 0) {
                            voxel.color |= (31 << 25);
                            chunk.setVoxel({scale, x, y, z}, voxel);
                        }
                    }
                }
            }
        }

        u32 chunk_size = 1u << scale;
        for (unsigned int x = 0; x < chunk_size; x++) {
            for (unsigned int z = 0; z < chunk_size; z++) {
                chunk.setVoxel({7, x, 0, z}, m_ground_material);
            }
        }

        return true;
    }
};

Unique<World> createWorldFromModel(Unique<VoxelModel> model, Voxel ground_voxel) {
    Unique<World> world = CreateUnique<World>(
            CreateUnique<SingleChunkModelChunkProvider>(std::move(model), ground_voxel),
            CreateUnique<ChunkStorage>(),
            threading::TickingThread::TicksPerSecond(20),
            ChunkSource::Settings());
    return world;
}

} //