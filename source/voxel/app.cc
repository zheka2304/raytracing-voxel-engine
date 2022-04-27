#include "app.h"

#include <iostream>
#include "voxel/common/profiler.h"


namespace voxel {

VoxelEngineApp::VoxelEngineApp() {
}

VoxelEngineApp::~VoxelEngineApp() {
}

void VoxelEngineApp::init() {
    m_engine.initialize();

    m_context = std::addressof(m_engine.newContext("APP"));
    m_context->setInitCallback([this] (Context& context, render::RenderContext& render_context) -> void { onInit(context, render_context); });
    m_context->setFrameHandleCallback([this] (Context& context, render::RenderContext& render_context) -> void { onFrame(context, render_context); });
    m_context->setEventProcessingCallback([this] (Context& context, WindowHandler& window_handler) -> void { onProcessEvents(context, window_handler); });
    m_context->setWindowResizeCallback([this] (Context& context, i32 w, i32 h) -> void { onWindowResize(context, w, h); });
    m_context->setWindowFocusCallback([this] (Context& context, i32 f) -> void { onWindowFocus(context, f); });
    m_context->setDestroyCallback([this] (Context& context) -> void { onDestroy(context); });

    m_world = createWorld();
    createWindow(*m_context);
}

void VoxelEngineApp::run() {
    m_world->setTicking(true);
    m_context->runEventLoop();
}

void VoxelEngineApp::joinEventLoopAndFinish() {
    m_engine.joinAllEventLoops();
    m_world->setTicking(false);
    m_world->joinTickingThread();
    m_world.release();
}


Unique<World> VoxelEngineApp::createWorld() {
    return nullptr;
}

void VoxelEngineApp::createWindow(Context& context) {

}

Unique<WorldRenderer> VoxelEngineApp::createWorldRenderer(Shared<ChunkSource> chunk_source) {
    return nullptr;
}


void VoxelEngineApp::onInit(Context& context, render::RenderContext& render_context) {
    m_world_renderer = createWorldRenderer(m_world->getChunkSource());
}

void VoxelEngineApp::onFrame(Context& context, render::RenderContext& render_context) {

}

void VoxelEngineApp::onProcessEvents(voxel::Context& ctx, voxel::WindowHandler& window_handler) {

}

void VoxelEngineApp::onWindowResize(voxel::Context& ctx, i32 width, i32 height) {
    glViewport(0, 0, width, height);
    setRenderTargetSize(width, height);
}

void VoxelEngineApp::onWindowFocus(voxel::Context& ctx, i32 focus) {

}

void VoxelEngineApp::onDestroy(voxel::Context& ctx) {
    // destroy graphics objects
    m_render_target.reset();
    m_full_screen_quad.reset();
    m_world_renderer.reset();
}


void VoxelEngineApp::setRenderTargetSize(i32 width, i32 height) {
    if (m_render_target && m_render_target->getWidth() == width && m_render_target->getHeight() == height) {
        return;
    }
    m_render_target = CreateUnique<render::RenderTarget>(width, height);
}

void VoxelEngineApp::renderMainCamera(render::RenderContext& render_context, render::Camera& camera) {
    VOXEL_ENGINE_PROFILE_GPU_SCOPE(render_all)

    if (!m_full_screen_quad) {
        m_full_screen_quad = CreateUnique<opengl::FullScreenQuad>();
    }

    {
        VOXEL_ENGINE_PROFILE_GPU_SCOPE(render_world)
        m_world_renderer->setCameraPosition(camera.getProjection().position);
        m_world_renderer->render(render_context);
    }

    {
        VOXEL_ENGINE_PROFILE_GPU_SCOPE(render_main_camera)
        camera.render(render_context, *m_render_target);
    }

    // render full screen quad with resulting texture
    VOXEL_ENGINE_SHADER_REF(voxel::opengl::GraphicsShader, basic_texture_shader, render_context.getShaderManager(), "basic_texture");
    VOXEL_ENGINE_SHADER_UNIFORM(uniform_texture_0, *basic_texture_shader, "TEXTURE_0");
    basic_texture_shader->bind();
    m_render_target->getResultTexture().bind(0, uniform_texture_0);
    m_full_screen_quad->render();
}

void BasicVoxelEngineApp::setupInput(input::SimpleInput& input) {
}

void BasicVoxelEngineApp::setupCamera(render::Camera& camera) {
}

void BasicVoxelEngineApp::updateCameraDimensions(render::Camera& camera, i32 width, i32 height) {
}

Unique<WorldRenderer> BasicVoxelEngineApp::createWorldRenderer(Shared<ChunkSource> chunk_source) {
    return CreateUnique<WorldRenderer>(chunk_source, CreateUnique<render::ChunkBuffer>(4096, 65536, math::Vec3i(32)), WorldRendererSettings());
}

void BasicVoxelEngineApp::createWindow(Context& context) {
    context.initWindow({1200, 800, "", {}});
}

void BasicVoxelEngineApp::onInit(Context& context, render::RenderContext& render_context) {
    VoxelEngineApp::onInit(context, render_context);

    m_camera = CreateUnique<render::Camera>();
    m_camera->getProjection().setFov(M_PI / 2.0f, 1.0);
    m_input = CreateUnique<input::SimpleInput>(context.getWindowHandler());
    m_input->getMouseControl().setMode(voxel::input::MouseControl::Mode::IN_GAME);

    setupCamera(*m_camera);
    setupInput(*m_input);
}

void BasicVoxelEngineApp::onFrame(Context& context, render::RenderContext& render_context) {
    VoxelEngineApp::onFrame(context, render_context);
    renderMainCamera(render_context, *m_camera);
}

void BasicVoxelEngineApp::onProcessEvents(Context& ctx, WindowHandler& window_handler) {
    VoxelEngineApp::onProcessEvents(ctx, window_handler);
    m_input->update(*m_camera);
}

void BasicVoxelEngineApp::onWindowResize(Context& ctx, i32 width, i32 height) {
    VoxelEngineApp::onWindowResize(ctx, width, height);
    updateCameraDimensions(*m_camera, width, height);
}

void BasicVoxelEngineApp::onDestroy(Context& ctx) {
    VoxelEngineApp::onDestroy(ctx);
    m_input.reset();
    m_camera.reset();
}


}