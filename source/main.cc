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


u32 getNormalBits(f32 x, f32 y, f32 z, f32 weight) {
    f32 xz = sqrt(x * x + z * z);
    f32 yaw = atan2(z, x);
    f32 pitch = atan2(y, xz);

    const i32 bit_offset = 8;
    return (u32((yaw + M_PI) / (M_PI * 2) * 63.0) |
            (u32((pitch / M_PI + 0.5) * 31.0) << 6) |
            (u32(weight * 15.0) << 11)) << bit_offset;
}

int main() {
    voxel::format::VoxFileFormat file_format;
    std::ifstream istream("models/vox/monu16.vox", std::ifstream::binary);
    auto models = file_format.read(istream);
    istream.close();
    auto model = models[0].get();
    std::cout << "model loaded\n";

    auto engine = std::make_shared<voxel::Engine>();
    engine->initialize();
    auto context = engine->newContext("ctx1");
    context->initWindow({900, 900, "test", {}});

    static voxel::render::Camera* camera = nullptr;
    static voxel::input::SimpleInput* simple_input = nullptr;

    context->setInitCallback([] (voxel::Context& ctx, voxel::render::RenderContext& render_context) {
        camera = new voxel::render::Camera();
        camera->getProjection().m_position = voxel::math::Vec3f(0, 2, 0);

        simple_input = new voxel::input::SimpleInput(ctx.getWindowHandler());
        simple_input->getMouseControl().setMode(voxel::input::MouseControl::Mode::IN_GAME);
        simple_input->setSensitivity(0.0025, 0.0025);
        simple_input->setMovementSpeed(0.025f);
    });

    context->setWindowResizeCallback([] (voxel::Context& ctx, int w, int h) -> void {
        std::cout << "window resize: " << w << ", " << h << "\n";
        glViewport(0, 0, w, h);
        camera->getProjection().setFov(60 * (180.0f / 3.1416f), h / (float) w);
    });

    context->setWindowFocusCallback([] (voxel::Context& ctx, int focus) -> void {
        std::cout << "window focus: " << focus << "\n";
    });

    context->setEventProcessingCallback([] (voxel::Context& ctx, voxel::WindowHandler& window_handler) {
        simple_input->update(*camera);
    });

    context->setFrameHandleCallback([model, context] (voxel::Context& ctx, voxel::render::RenderContext& render_context) {
        static voxel::opengl::FullScreenQuad* quad = nullptr;
        if (!quad) {
            quad = new voxel::opengl::FullScreenQuad();
        }

        static voxel::render::RenderTarget* render_target = nullptr;
        static voxel::render::ChunkBuffer* chunk_buffer = nullptr;
        if (!render_target) {
            render_target = new voxel::render::RenderTarget(900, 900);

            auto chunk = voxel::CreateShared<voxel::Chunk>(voxel::ChunkPosition({ 0, 0, 0 }));
//            for (u32 x = 0; x < 64; x++) {
//                for (u32 z = 0; z < 64; z++) {
//                    for (u32 y = 0; y < 64; y ++) {
//                        int dx = int(x) - 32;
//                        int dy = int(y) - 32;
//                        int dz = int(z) - 32;
//                        if (dx * dx + dy * dy + dz * dz < 32 * 32) {
//                            chunk->setVoxel({6, x, y, z}, { (31 << 25) | 0x00FFFF, getNormalBits(dx, dy, dz, 0.5) });
//                        }
//                    }
//                }
//            }
//            chunk->setVoxel({1, 0, 0, 0}, { (31 << 25) | 0xFFFF00, 0 });
//            chunk->setVoxel({1, 0, 0, 1}, { (31 << 25) | 0xFFFF00, 0 });
//            chunk->setVoxel({1, 1, 0, 0}, { (31 << 25) | 0xFFFF00, 0 });
//            chunk->setVoxel({1, 1, 0, 1}, { (31 << 25) | 0xFFFF00, 0 });

            auto size = model->getSize();
            for (unsigned int x = 0; x < size.x; x++) {
                for (unsigned int y = 0; y < size.y; y++) {
                    for (unsigned int z = 0; z < size.z; z++) {
                        voxel::Voxel voxel = model->getVoxel(x, y, z);
                        voxel.material = 0;
                        if (voxel.color != 0) {
                            voxel.color |= (31 << 25);
                            chunk->setVoxel({7, x, y, z}, voxel);
                        }
                    }
                }
            }

            for (unsigned int x = 0; x < 128; x++) {
                for (unsigned int z = 0; z < 128; z++) {
                    chunk->setVoxel({7, x, 0, z}, { (31 << 25) | 0x77FFCC, 0 });
                }
            }

            if (!chunk_buffer) {
                chunk_buffer = new voxel::render::ChunkBuffer(256, voxel::math::Vec3i(50, 50, 50));
            }

            chunk_buffer->uploadChunk(chunk);
            chunk_buffer->rebuildChunkMap(voxel::math::Vec3i(0));
//            auto buffer = new voxel::opengl::ShaderStorageBuffer("world.chunk_data_buffer");
//            buffer->setData(chunk->getBufferSize() * 4, (void*) chunk->getBuffer(), GL_STATIC_DRAW);
//            buffer->bind(render_context.getShaderManager());
        }

        static voxel::render::FetchedChunksList fetched_chunks_list;
        chunk_buffer->getFetchedChunks(fetched_chunks_list);
        fetched_chunks_list.runDataUpdate(1024);
        std::cout << fetched_chunks_list.getChunksToFetch().size() << "\n";
        chunk_buffer->prepareAndBind(render_context);

        camera->render(render_context, *render_target);

        VOXEL_ENGINE_SHADER_REF(voxel::opengl::GraphicsShader, basic_texture_shader, render_context.getShaderManager(), "basic_texture");
        VOXEL_ENGINE_SHADER_UNIFORM(uniform_texture_0, *basic_texture_shader, "TEXTURE_0");
        basic_texture_shader->bind();
        render_target->getResultTexture().bind(0, uniform_texture_0);
        quad->render();

        voxel::utils::Stopwatch stopwatch;
        glFinish();

        static int frame_counter = 0;
        static long long last_frame_timestamp = voxel::utils::getTimestampMillis();
        const int frames_per_measure = 30;
        if (frame_counter % frames_per_measure == 0) {
            long long timestamp = voxel::utils::getTimestampMillis();
            std::stringstream ss;
            ss << "fps: " << int(float(frames_per_measure) / (timestamp - last_frame_timestamp) * 1000.0);
            ss << " frame time: " <<  int((timestamp - last_frame_timestamp) / float(frames_per_measure)) << " ms";
            glfwSetWindowTitle(context->getGlfwWindow(), ss.str().data());
            last_frame_timestamp = timestamp;
        }

        frame_counter++;

//         std::this_thread::sleep_for(std::chrono::milliseconds(250));
    });

    context->runEventLoop();
    engine->joinAllEventLoops();

    return 0;
}