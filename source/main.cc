#include <iostream>

#include "voxel/common/profiler.h"
#include "voxel/app.h"
#include "voxel/premade.h"


using namespace voxel;

class MyVoxelEngineApp : public BasicVoxelEngineApp {
    void setupInput(input::SimpleInput& input) override {
        input.getMouseControl().setMode(input::MouseControl::Mode::IN_GAME);
        input.setSensitivity(0.0025, 0.0025);
        input.setMovementSpeed(0.025f);
    }

    void setupCamera(render::Camera& camera) override {
        camera.getProjection().position = math::Vec3f(0, 1, 0);
        camera.getProjection().setFov(90 / (180.0f / 3.1416f), 1);
    }

    void updateCameraDimensions(render::Camera &camera, i32 width, i32 height) override {
        camera.getProjection().setFov(90 / (180.0f / 3.1416f), height / (float) width);
    }

    virtual Unique<World> createWorld() override {
        return createWorldFromModel(
                loadVoxelModelFromVoxFile("assets/models/vox/monu7.vox"),
                { (31 << 25) | 0x77FFCC, 0 });
    }

    void onFrame(Context& context, render::RenderContext& render_context) override {
        BasicVoxelEngineApp::onFrame(context, render_context);
        glfwSetWindowTitle(context.getGlfwWindow(), Profiler::getWindowTitlePerformanceStats().data());
    }
};


int main() {
    {
        MyVoxelEngineApp app;
        app.init();
        app.run();
        app.joinEventLoopAndFinish();
    }

    std::cout << "\n--- PROFILER INFO: ---\n" << Profiler::getAppProfilerStats();

    return 0;
}