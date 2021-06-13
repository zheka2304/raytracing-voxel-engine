#ifndef VOXEL_ENGINE_CONTEXT_H
#define VOXEL_ENGINE_CONTEXT_H

#include <GLFW/glfw3.h>

struct WindowParams {
public:
    const char* title;
    int width;
    int height;
};

class RenderContext {
private:
    GLFWwindow* window;
    bool _hasAttachedWindow;

public:
    RenderContext(WindowParams, RenderContext* shared = nullptr);
    RenderContext(RenderContext* shared = nullptr);
    ~RenderContext();

    void makeCurrent();
    void swapBuffers();
    bool hasAttachedWindow();
    GLFWwindow* getWindow();
};

#endif //VOXEL_ENGINE_CONTEXT_H
