
#ifndef VOXEL_ENGINE_ENGINE_H
#define VOXEL_ENGINE_ENGINE_H

#include <memory>
#include "common/worker_thread.h"
#include "renderer/context.h"


class VoxelEngine {
public:
    RenderContext windowContext;
    RenderContext gpuWorkerThreadContext;

private:
    WorkerThread gpuWorkerThread;

public:
    explicit VoxelEngine(WindowParams windowParams);

    void runOnGpuWorkerThread(std::function<void()> const& task);
    void swapGpuWorkerBuffers();
    GLFWwindow* getWindow();
};

#endif //VOXEL_ENGINE_ENGINE_H
