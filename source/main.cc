#include <iostream>
#include <sstream>
#include <fstream>

#include "voxel/engine/engine.h"
#include "voxel/engine/input/simple_input.h"
#include "voxel/engine/render/camera.h"
#include "voxel/engine/render/chunk_buffer.h"
#include "voxel/engine/world/chunk.h"
#include "voxel/common/utils/time.h"
#include "voxel/engine/file/vox_file_format.h"
#include "voxel/engine/file/riff_file_format.h"
#include "voxel/engine/world/chunk_provider.h"
#include "voxel/engine/world/chunk_storage.h"
#include "voxel/engine/world/chunk_source.h"
#include "voxel/engine/world/world_renderer.h"
#include "voxel/engine/world.h"
#include "voxel/common/profiler.h"


u32 getNormalBits(f32 x, f32 y, f32 z, f32 weight) {
    f32 xz = sqrt(x * x + z * z);
    f32 yaw = atan2(z, x);
    f32 pitch = atan2(y, xz);

    const i32 bit_offset = 8;
    return (u32((yaw + M_PI) / (M_PI * 2) * 63.0) |
            (u32((pitch / M_PI + 0.5) * 31.0) << 6) |
            (u32(weight * 15.0) << 11)) << bit_offset;
}


namespace voxel {
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

//        i64 time_start = voxel::utils::getTimestampMillis();
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

        srand(voxel::utils::getTimestampNanos());
        i32 offset_x = rand() % 1;
        i32 offset_z = rand() % 1;

        auto size = m_model.getSize();
        for (unsigned int x = 0; x < size.x; x++) {
            for (unsigned int y = 0; y < size.y; y++) {
                for (unsigned int z = 0; z < size.z; z++) {
                    voxel::Voxel voxel = m_model.getVoxel(x, y, z);
                    voxel.material = 0;
                    if (voxel.color != 0) {
                        voxel.color |= (31 << 25);
                        chunk->setVoxel({7, x + offset_x, y, z + offset_z}, voxel);
                    }
                }
            }
        }

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

//        i64 time_end = voxel::utils::getTimestampMillis();
//        debug_time_generation += time_end - time_start;
//        std::cout << "efficient time ratio: " << debug_time_generation / f32(time_end - debug_time_start) << "\n";


        return true;
    }
};
}

int main() {

    voxel::VoxelModel* model;
    voxel::format::VoxFileFormat file_format;
    std::ifstream istream("models/vox/monu16.vox", std::ifstream::binary);

    VOXEL_ENGINE_PROFILE_SCOPE(startup_model_load);
    auto models = file_format.read(istream);
    istream.close();
    model = models[0].get();
    VOXEL_ENGINE_PROFILE_SCOPE_END(startup_model_load);

    auto engine = std::make_shared<voxel::Engine>();
    engine->initialize();
    auto context = engine->newContext("ctx1");
    context->initWindow({1920, 1080, "test", {}});

    voxel::Shared<voxel::ChunkProvider> chunk_provider = voxel::CreateShared<voxel::DebugChunkProvider>(*model);
    voxel::Shared<voxel::ChunkStorage> chunk_storage = voxel::CreateShared<voxel::ChunkStorage>();
    voxel::Shared<voxel::World> world = voxel::CreateShared<voxel::World>(
            chunk_provider, chunk_storage,
            voxel::threading::TickingThread::TicksPerSecond(20),
            voxel::ChunkSource::Settings());

    world->setTicking(true);

    static voxel::render::Camera* camera = nullptr;
    static voxel::input::SimpleInput* simple_input = nullptr;

    context->setInitCallback([] (voxel::Context& ctx, voxel::render::RenderContext& render_context) {
        camera = new voxel::render::Camera();
        camera->getProjection().position = voxel::math::Vec3f(0, 1, 0);

        simple_input = new voxel::input::SimpleInput(ctx.getWindowHandler());
        simple_input->getMouseControl().setMode(voxel::input::MouseControl::Mode::IN_GAME);
        simple_input->setSensitivity(0.0025, 0.0025);
        simple_input->setMovementSpeed(0.025f);
    });

    context->setWindowResizeCallback([] (voxel::Context& ctx, int w, int h) -> void {
        std::cout << "window resize: " << w << ", " << h << "\n";
        glViewport(0, 0, w, h);
        camera->getProjection().setFov(90 / (180.0f / 3.1416f), h / (float) w);
    });

    context->setWindowFocusCallback([] (voxel::Context& ctx, int focus) -> void {
        std::cout << "window focus: " << focus << "\n";
    });

    context->setEventProcessingCallback([] (voxel::Context& ctx, voxel::WindowHandler& window_handler) {
        simple_input->update(*camera);
    });

    context->setFrameHandleCallback([model, world, context] (voxel::Context& ctx, voxel::render::RenderContext& render_context) {
        VOXEL_ENGINE_PROFILE_GPU_SCOPE(render_all)

        static voxel::opengl::FullScreenQuad* quad = nullptr;
        if (!quad) {
            quad = new voxel::opengl::FullScreenQuad();
        }

        static voxel::render::RenderTarget* render_target = nullptr;
        static voxel::WorldRenderer* world_renderer = nullptr;
        if (!render_target) {
            render_target = new voxel::render::RenderTarget(1920, 1080);
            world_renderer = new voxel::WorldRenderer(world->getChunkSource(), voxel::CreateShared<voxel::render::ChunkBuffer>(4096, 65536, voxel::math::Vec3i(50)), voxel::WorldRendererSettings());
        }

        {
            VOXEL_ENGINE_PROFILE_GPU_SCOPE(render_world)
            world_renderer->setCameraPosition(camera->getProjection().position);
            world_renderer->render(render_context);
        }

        {
            VOXEL_ENGINE_PROFILE_GPU_SCOPE(render_main_camera)
            camera->render(render_context, *render_target);
        }

        VOXEL_ENGINE_SHADER_REF(voxel::opengl::GraphicsShader, basic_texture_shader, render_context.getShaderManager(), "basic_texture");
        VOXEL_ENGINE_SHADER_UNIFORM(uniform_texture_0, *basic_texture_shader, "TEXTURE_0");
        basic_texture_shader->bind();
        render_target->getResultTexture().bind(0, uniform_texture_0);
        quad->render();

        std::stringstream ss;
        ss << "fps: " << int(1000.0 / voxel::Profiler::get().getAverageValue("render_all"));
        ss << " frame time: " << voxel::Profiler::get().getAverageValue("render_main_camera") << "ms";
        ss << " world time: " << voxel::Profiler::get().getAverageValue("render_world") << "ms";
        glfwSetWindowTitle(context->getGlfwWindow(), ss.str().data());

//         std::this_thread::sleep_for(std::chrono::milliseconds(250));
    });

    context->runEventLoop();
    engine->joinAllEventLoops();

    std::cout << "Profiler data:\n" <<
        "  full frame: " << voxel::Profiler::get().getAverageValue("render_all") << " ms\n" <<
        "    raytrace: " << voxel::Profiler::get().getAverageValue("render_raytrace_pass") << " ms\n" <<
        "    lightmap: " << voxel::Profiler::get().getAverageValue("render_lightmap") << " ms\n" <<
        "      interpolate: " << voxel::Profiler::get().getAverageValue("render_lightmap_interpolate") << " ms\n" <<
        "      blur: " << voxel::Profiler::get().getAverageValue("render_lightmap_blur") << " ms\n" <<
        "      3x3: " << voxel::Profiler::get().getAverageValue("render_lightmap_3x3") << " ms\n" <<
        "    spatial buffer: " << voxel::Profiler::get().getAverageValue("render_spatial_buffer") << " ms\n" <<
        "      reset: " << voxel::Profiler::get().getAverageValue("render_spatial_buffer_reset") << " ms\n" <<
        "      pass: " << voxel::Profiler::get().getAverageValue("render_spatial_buffer_pass") << " ms\n" <<
        "      postprocess: " << voxel::Profiler::get().getAverageValue("render_spatial_buffer_postprocess") << " ms\n" <<
        "    postprocess and output: " << voxel::Profiler::get().getAverageValue("render_postprocess_output") << " ms\n" <<
        "    world: " << voxel::Profiler::get().getAverageValue("render_world") << " ms\n";

    return 0;
}