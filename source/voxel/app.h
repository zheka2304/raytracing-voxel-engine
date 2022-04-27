#ifndef VOXEL_ENGINE_APP_H
#define VOXEL_ENGINE_APP_H

#include <functional>
#include "voxel/common/base.h"
#include "voxel/common/opengl.h"
#include "voxel/engine/engine.h"
#include "voxel/engine/world.h"
#include "voxel/engine/world/world_renderer.h"
#include "voxel/engine/render/camera.h"
#include "voxel/engine/render/render_context.h"
#include "voxel/engine/render/render_target.h"
#include "voxel/engine/input/simple_input.h"


namespace voxel {

class VoxelEngineApp {
public:
    VoxelEngineApp();
    ~VoxelEngineApp();

    void init();
    void run();
    void joinEventLoopAndFinish();

protected:
    virtual Unique<World> createWorld();
    virtual Unique<WorldRenderer> createWorldRenderer(Shared<ChunkSource> chunk_source);
    virtual void createWindow(Context& context);

    virtual void onInit(Context& context, render::RenderContext& render_context);
    virtual void onFrame(Context& context, render::RenderContext& render_context);
    virtual void onProcessEvents(Context& ctx, WindowHandler& window_handler);
    virtual void onWindowResize(Context& ctx, i32 width, i32 height);
    virtual void onWindowFocus(Context& ctx, i32 focus);
    virtual void onDestroy(Context& ctx);

    void renderMainCamera(render::RenderContext& render_context, render::Camera& camera);
    void setRenderTargetSize(i32 width, i32 height);

private:
    Engine m_engine;
    Context* m_context;
    Unique<World> m_world;

    Unique<WorldRenderer> m_world_renderer;
    Unique<render::RenderTarget> m_render_target;
    Unique<opengl::FullScreenQuad> m_full_screen_quad;
};

class BasicVoxelEngineApp : public VoxelEngineApp {
    Unique<render::Camera> m_camera;
    Unique<input::SimpleInput> m_input;

protected:
    virtual void setupInput(input::SimpleInput& input);
    virtual void setupCamera(render::Camera& camera);
    virtual void updateCameraDimensions(render::Camera& camera, i32 width, i32 height);

    virtual Unique<WorldRenderer> createWorldRenderer(Shared<ChunkSource> chunk_source) override;
    virtual void createWindow(Context& context) override;

    void onInit(Context &context, render::RenderContext& render_context) override;
    void onFrame(Context &context, render::RenderContext& render_context) override;
    void onProcessEvents(Context& ctx, WindowHandler& window_handler) override;
    void onWindowResize(Context& ctx, i32 width, i32 height) override;
    void onDestroy(Context& ctx) override;
};

} // voxel

#endif //VOXEL_ENGINE_APP_H
