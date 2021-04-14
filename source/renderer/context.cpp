#include "context.h"


RenderContext::RenderContext(RenderContext* shared) {
    _hasAttachedWindow = false;
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    window = glfwCreateWindow(640, 480, "", nullptr, shared != nullptr ? shared->window : nullptr);
}

RenderContext::RenderContext(WindowParams params, RenderContext* shared) {
    _hasAttachedWindow = true;
    glfwDefaultWindowHints();
    window = glfwCreateWindow(params.width, params.height, params.title, nullptr, shared != nullptr ? shared->window : nullptr);
}

RenderContext::~RenderContext() {
    glfwDestroyWindow(window);
}

void RenderContext::makeCurrent() {
    glfwMakeContextCurrent(window);
}

void RenderContext::swapBuffers() {
    glfwSwapBuffers(window);
}

bool RenderContext::hasAttachedWindow() {
    return _hasAttachedWindow;
}

GLFWwindow* RenderContext::getWindow() {
    return window;
}
