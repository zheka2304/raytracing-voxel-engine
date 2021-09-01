#include <iostream>
#include "voxel/engine/engine.h"
#include "voxel/engine/input/simple_input.h"
#include "voxel/engine/render/camera.h"
#include "voxel/engine/world/chunk.h"
#include "voxel/common/utils/time.h"


int main() {

    auto engine = std::make_shared<voxel::Engine>();
    engine->initialize();
    auto context = engine->newContext("ctx1");
    context->initWindow({640, 640, "test", {}});

    static voxel::render::Camera* camera = nullptr;
    static voxel::input::SimpleInput* simple_input = nullptr;

    context->setInitCallback([] (voxel::Context& ctx, voxel::render::RenderContext& render_context) {
        camera = new voxel::render::Camera();
        camera->getProjection().m_position = voxel::math::Vec3f(0, 2, 0);

        simple_input = new voxel::input::SimpleInput(ctx.getWindowHandler());
        simple_input->getMouseControl().setMode(voxel::input::MouseControl::Mode::IN_GAME);
        simple_input->setMovementSpeed(0.01f);
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

    context->setFrameHandleCallback([] (voxel::Context& ctx, voxel::render::RenderContext& render_context) {
        static voxel::opengl::FullScreenQuad* quad = nullptr;
        if (!quad) {
            quad = new voxel::opengl::FullScreenQuad();
        }

        static voxel::render::RenderTarget* render_target = nullptr;
        if (!render_target) {
            render_target = new voxel::render::RenderTarget(1024, 1024);

            auto chunk = new voxel::world::Chunk({ 0, 0, 0 });
            // chunk->preallocate(5000, 5000);
            for (voxel::u32 x = 0; x < 64; x++) {
                for (voxel::u32 z = 0; z < 64; z++) {
                    for (voxel::u32 y = 0; y < (rand() & 3); y ++) {
                        chunk->setVoxel({6, x, y, z});
                    }
                }
            }

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
    });

    context->runEventLoop();
    engine->joinAllEventLoops();

    return 0;
}