#include "engine.h"

VoxelEngine::VoxelEngine(WindowParams windowParams) :
    windowContext(windowParams),
    gpuWorkerThreadContext(&windowContext) {
    gpuWorkerThread.queue([&] () -> void { gpuWorkerThreadContext.makeCurrent(); });
}

void VoxelEngine::runOnGpuWorkerThread(std::function<void()> const& task) {
    gpuWorkerThread.queue(task);
}

void VoxelEngine::swapGpuWorkerBuffers() {
    gpuWorkerThread.queue([=] () -> void { gpuWorkerThreadContext.swapBuffers(); });
}

GLFWwindow* VoxelEngine::getWindow() {
    return windowContext.getWindow();
}