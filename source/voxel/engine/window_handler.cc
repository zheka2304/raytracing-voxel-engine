#include "window_handler.h"

namespace voxel {

    void IWindowHandlerListener::onWindowFocusGained() {
    }

    void IWindowHandlerListener::onWindowFocusLost() {
    }

    void IWindowHandlerListener::onWindowHandlerDestroyed() {
    }


    WindowHandler::WindowHandler(GLFWwindow* window) : m_window(window) {
    }

    WindowHandler::~WindowHandler() {
        for (auto listener : m_listeners) {
            listener->onWindowHandlerDestroyed();
        }
    }

    void WindowHandler::update() {
        i32 focus = glfwGetWindowAttrib(m_window, GLFW_FOCUSED);
        if (m_window_focus != focus) {
            m_window_focus = focus;
            if (!m_window_focus) {
                for (auto listener : m_listeners) {
                    listener->onWindowFocusLost();
                }
                m_focused = false;
            }
        }

        if (!m_focused) {
            if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
                for (auto listener : m_listeners) {
                    listener->onWindowFocusGained();
                }
                m_focused = true;
            }
        }
    }

    void WindowHandler::addListener(IWindowHandlerListener* listener) {
        m_listeners.insert(listener);
    }

    void WindowHandler::removeListener(IWindowHandlerListener* listener) {
        m_listeners.erase(listener);
    }

    GLFWwindow* WindowHandler::getWindow() {
        return m_window;
    }

    bool WindowHandler::isFocused() {
        return m_focused;
    }

    bool WindowHandler::wasFocused() {
        return m_was_focused;
    }

    i32 WindowHandler::getWindowFocus() {
        return m_window_focus;
    }


} // voxel
