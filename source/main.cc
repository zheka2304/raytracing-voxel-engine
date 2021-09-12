#include <iostream>
#include <sstream>

#include "voxel/engine/engine.h"
#include "voxel/engine/input/simple_input.h"
#include "voxel/engine/render/camera.h"
#include "voxel/engine/world/chunk.h"
#include "voxel/common/utils/time.h"

voxel::u32 getNormalBits(voxel::f32 x, voxel::f32 y, voxel::f32 z, voxel::f32 weight) {
    voxel::f32 xz = sqrt(x * x + z * z);
    voxel::f32 yaw = atan2(z, x);
    voxel::f32 pitch = atan2(y, xz);

    const voxel::i32 bit_offset = 8;
    return (voxel::u32((yaw + M_PI) / (M_PI * 2) * 63.0) |
            (voxel::u32((pitch / M_PI + 0.5) * 31.0) << 6) |
            (voxel::u32(weight * 15.0) << 11)) << bit_offset;
}

int main() {

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

    context->setFrameHandleCallback([context] (voxel::Context& ctx, voxel::render::RenderContext& render_context) {
        static voxel::opengl::FullScreenQuad* quad = nullptr;
        if (!quad) {
            quad = new voxel::opengl::FullScreenQuad();
        }

        static voxel::render::RenderTarget* render_target = nullptr;
        if (!render_target) {
            render_target = new voxel::render::RenderTarget(900, 900);

            auto chunk = new voxel::world::Chunk({ 0, 0, 0 });
//            chunk->preallocate(21512, 137352);
            for (voxel::u32 x = 0; x < 64; x++) {
                for (voxel::u32 z = 0; z < 64; z++) {
                    for (voxel::u32 y = 0; y < 64; y ++) {
                        int dx = int(x) - 32;
                        int dy = int(y) - 32;
                        int dz = int(z) - 32;
                        if (dx * dx + dy * dy + dz * dz < 32 * 32) {
                            chunk->setVoxel({6, x, y, z}, (31 << 25) | 0x00FFFF, getNormalBits(dx, dy, dz, 0.5));
                        }
                    }
                }
            }
            chunk->setVoxel({1, 0, 0, 0}, (16 << 25) | 0xFFFF00, 0);

            auto buffer = new voxel::opengl::ShaderStorageBuffer("raytrace.voxel_buffer");
            buffer->setData(chunk->getBufferSize() * 4, (void*) chunk->getBuffer(), GL_STATIC_DRAW);
            buffer->bind(render_context.getShaderManager());
        }

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

        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    context->runEventLoop();
    engine->joinAllEventLoops();

    return 0;
}