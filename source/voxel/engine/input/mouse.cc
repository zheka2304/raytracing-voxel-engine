#include "mouse.h"


namespace voxel {
namespace input {

MouseControl::MouseControl(WindowHandler& window_handler) : m_window_handler(&window_handler) {
    m_window_handler->addListener(this);
}

MouseControl::~MouseControl() {
    _removeMode(m_mode);
    if (m_window_handler != nullptr) {
        m_window_handler->removeListener(this);
    }
}

void MouseControl::_initMode(Mode mode) {
    GLFWwindow* window = m_window_handler->getWindow();
    switch (mode) {
        case Mode::IN_GAME: {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

            int w, h;
            glfwGetWindowSize(window, &w, &h);
            m_last_pos.x = m_current_pos.x = w / 2.0f;
            m_last_pos.y = m_current_pos.y = h / 2.0f;
            glfwSetCursorPos(window, m_current_pos.x, m_current_pos.y);
            break;
        }

        case Mode::IN_UI: {
            double x, y;
            glfwGetCursorPos(window, &x, &y);
            m_last_pos.x = m_current_pos.x = x;
            m_last_pos.y = m_current_pos.y = y;
            break;
        }

        case Mode::RELEASED: {
            break;
        }
    }
}

void MouseControl::_removeMode(Mode mode) {
    GLFWwindow* window = m_window_handler->getWindow();
    switch (mode) {
        case Mode::IN_GAME:
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            break;

        case Mode::IN_UI:
            break;

        case Mode::RELEASED:
            break;
    }
}

void MouseControl::onWindowFocusGained() {
    _initMode(m_mode);
}

void MouseControl::onWindowFocusLost() {
    _removeMode(m_mode);
}

void MouseControl::onWindowHandlerDestroyed() {
    m_window_handler = nullptr;
}

void MouseControl::setMode(Mode mode) {
    if (mode != m_mode) {
        _removeMode(m_mode);
        m_mode = mode;
        _initMode(m_mode);
    }
}

void MouseControl::update() {
    if (m_window_handler == nullptr || !m_window_handler->isFocused()) {
        return;
    }

    GLFWwindow* window = m_window_handler->getWindow();

    m_last_pos = m_current_pos;

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    m_current_pos.x = x;
    m_current_pos.y = y;

    switch (m_mode) {
        case Mode::IN_GAME: {
            int w, h;
            glfwGetWindowSize(window, &w, &h);
            glfwSetCursorPos(window, m_last_pos.x = w / 2.0f, m_last_pos.y = h / 2.0f);
            break;
        }

        case Mode::IN_UI:
            break;

        case Mode::RELEASED:
            break;
    }
}

math::Vec2 MouseControl::getLastPos() {
    return m_last_pos;
}

math::Vec2 MouseControl::getMousePos() {
    return m_current_pos;
}

math::Vec2 MouseControl::getMouseMove() {
    return m_current_pos - m_last_pos;
}

int MouseControl::getMouseButton(int button) {
    if (m_window_handler == nullptr) {
        return 0;
    }
    return glfwGetMouseButton(m_window_handler->getWindow(), button);
}

} // input
} // voxel
