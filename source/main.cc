#include <iostream>
#include <sstream>
#include <fstream>

#include "voxel/common/base.h"
#include "voxel/common/profiler.h"
#include "voxel/common/utils/time.h"

#include "voxel/engine/world.h"
#include "voxel/engine/world/chunk.h"
#include "voxel/engine/render/chunk_buffer.h"
#include "voxel/engine/world/chunk_provider.h"
#include "voxel/engine/world/chunk_storage.h"
#include "voxel/engine/world/chunk_source.h"
#include "voxel/engine/world/world_renderer.h"
#include "voxel/engine/file/vox_file_format.h"

#include "voxel/app.h"


using namespace voxel;

class DebugChunkProvider : public ChunkProvider {
    VoxelModel m_model;
    std::atomic<i32> m_generated_count = 0;

public:
    DebugChunkProvider(const VoxelModel& model) : m_model(model) {
    }

    bool canFetchChunk(ChunkSource& chunk_source, ChunkPosition position) override {
        return position.y == 0; // && ((position.x + position.z) & 1);
    }

    Shared<Chunk> createChunk(ChunkSource &chunk_source, ChunkPosition position) override {
        return CreateShared<Chunk>(position);
    }

    bool buildChunk(ChunkSource &chunk_source, Shared<Chunk> chunk) override {
        m_generated_count++;

//        i64 time_start = utils::getTimestampMillis();
//        if (debug_time_start == 0) {
//            debug_time_start = time_start;
//        }

//        for (u32 x = 0; x < 64; x++) {
//            for (u32 z = 0; z < 64; z++) {
//                for (u32 y = 0; y < 64; y ++) {
//                    int dx = int(x) - 32;
//                    int dy = int(y) - 32;
//                    int dz = int(z) - 32;
//                    if (dx * dx + dy * dy + dz * dz < 32 * 32) {
//                        chunk->setVoxel({6, x, y, z}, { (31 << 25) | 0x00FFFF, getNormalBits(dx, dy, dz, 0.5) });
//                    }
//                }
//            }
//        }
//        chunk->setVoxel({1, 0, 0, 0}, { (31 << 25) | 0xFFFF00, 0 });
//        chunk->setVoxel({1, 0, 0, 1}, { (31 << 25) | 0xFFFF00, 0 });
//        chunk->setVoxel({1, 1, 0, 0}, { (31 << 25) | 0xFFFF00, 0 });
//        chunk->setVoxel({1, 1, 0, 1}, { (31 << 25) | 0xFFFF00, 0 });

        srand(utils::getTimestampNanos());
        i32 offset_x = rand() % 1;
        i32 offset_z = rand() % 1;

//        auto size = m_model.getSize();
//        for (unsigned int x = 0; x < size.x; x++) {
//            for (unsigned int y = 0; y < size.y; y++) {
//                for (unsigned int z = 0; z < size.z; z++) {
//                    Voxel voxel = m_model.getVoxel(x, y, z);
//                    voxel.material = 0;
//                    if (voxel.color != 0) {
//                        voxel.color |= (31 << 25);
//                        chunk->setVoxel({7, x + offset_x, y, z + offset_z}, voxel);
//                    }
//                }
//            }
//        }

        u32 grass_mat = 0;//getNormalBits(0.0, 1.0, 0.0, 1.0);
        for (unsigned int x = 0; x < 256; x++) {
            for (unsigned int z = 0; z < 256; z++) {
                // if ((x + z) & 1)
                chunk->setVoxel({8, x, 0, z}, {(31 << 25) | 0x77FFCC, grass_mat});
//                for (unsigned int i = 0; i < rand() % 4; i++) {
//                    chunk->setVoxel({8, x, i + 1, z}, {(31 << 25) | 0x77FFCC, grass_mat});
//                }
            }
        }

//        i64 time_end = utils::getTimestampMillis();
//        debug_time_generation += time_end - time_start;
//        std::cout << "efficient time ratio: " << debug_time_generation / f32(time_end - debug_time_start) << "\n";


        return true;
    }
};


namespace voxel {

class MyVoxelEngineApp : public BasicVoxelEngineApp {
    void setupInput(input::SimpleInput& input) override {
        input.getMouseControl().setMode(input::MouseControl::Mode::IN_GAME);
        input.setSensitivity(0.0025, 0.0025);
        input.setMovementSpeed(0.25f);
    }

    void setupCamera(render::Camera& camera) override {
        camera.getProjection().position = math::Vec3f(0, 1, 0);
        camera.getProjection().setFov(90 / (180.0f / 3.1416f), 1);
    }

    void updateCameraDimensions(render::Camera &camera, i32 width, i32 height) override {
        camera.getProjection().setFov(90 / (180.0f / 3.1416f), 1);
    }

    virtual Unique<World> createWorld() override {
        VoxelModel* model;
        format::VoxFileFormat file_format;
        std::ifstream istream("models/vox/monu16.vox", std::ifstream::binary);
        auto models = file_format.read(istream);
        istream.close();
        model = models[0].get();

        Shared<ChunkProvider> chunk_provider = CreateShared<DebugChunkProvider>(*model);
        Shared<ChunkStorage> chunk_storage = CreateShared<ChunkStorage>();
        Unique<World> world = CreateUnique<World>(
                chunk_provider, chunk_storage,
                threading::TickingThread::TicksPerSecond(20),
                ChunkSource::Settings());
        return world;
    }

    virtual Unique<WorldRenderer> createWorldRenderer(Shared<ChunkSource> chunk_source) override {
        return CreateUnique<WorldRenderer>(chunk_source, CreateShared<render::ChunkBuffer>(4096, 65536, math::Vec3i(32)), WorldRendererSettings());
    }

    virtual void createWindow(Context& context) override {
        context.initWindow({800, 600, "", {}});
    }

    void onFrame(Context& context, render::RenderContext& render_context) override {
        BasicVoxelEngineApp::onFrame(context, render_context);
        glfwSetWindowTitle(context.getGlfwWindow(), Profiler::getWindowTitlePerformanceStats().data());
    }
};

}


int main() {
    MyVoxelEngineApp app;
    app.init();
    app.run();
    app.joinEventLoopAndFinish();

    std::cout << "\n--- PROFILER INFO: ---\n" << Profiler::getAppProfilerStats();

    return 0;
}