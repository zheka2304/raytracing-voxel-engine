#include <iostream>
#include "voxel/engine/engine.h"
#include "voxel/engine/render/camera.h"


int main() {


    auto engine = std::make_shared<voxel::Engine>();
    engine->initialize();
    auto context = engine->newContext("ctx1");
    context->initWindow({640, 480, "test", {}});

    context->setWindowResizeCallback([] (voxel::Context& ctx, int w, int h) -> void {
        std::cout << "window resize: " << w << ", " << h << "\n";
        glViewport(0, 0, w, h);
    });

    context->setWindowFocusCallback([] (voxel::Context& ctx, int focus) -> void { std::cout << "window focus: " << focus << "\n"; });
    context->setFrameHandleCallback([] (voxel::Context& ctx, voxel::render::RenderContext& render_context) {
        static voxel::opengl::FullScreenQuad* quad = nullptr;
        if (!quad) {
            quad = new voxel::opengl::FullScreenQuad();
        }

        static voxel::render::Camera* camera = nullptr;
        if (!camera) {
            camera = new voxel::render::Camera();
        }

        static voxel::render::RenderTarget* render_target = nullptr;
        if (!render_target) {
            render_target = new voxel::render::RenderTarget(256, 256);
        }

        camera->render(render_context, *render_target);

        VOXEL_ENGINE_SHADER_REF(voxel::opengl::GraphicsShader, basic_texture_shader, render_context.getShaderManager(), "basic_texture");
        VOXEL_ENGINE_SHADER_UNIFORM(uniform_texture_0, *basic_texture_shader, "TEXTURE_0");
        basic_texture_shader->bind();
        render_target->getResultTexture().bind(0, uniform_texture_0);
        quad->render();

    });

    context->runEventLoop();
    engine->joinAllEventLoops();

    return 0;
}