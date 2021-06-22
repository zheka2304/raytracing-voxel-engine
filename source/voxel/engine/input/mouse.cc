#include "mouse.h"


namespace voxel {
namespace input {

MouseControl::MouseControl(GLFWwindow* window) : m_window(window) {
}

MouseControl::~MouseControl() {
    _removeMode(m_mode);
}

void MouseControl::_initMode(Mode mode) {
    switch (mode) {
        case Mode::IN_GAME: {
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

            int w, h;
            glfwGetWindowSize(m_window, &w, &h);
            m_last_pos.x = m_current_pos.x = w / 2.0f;
            m_last_pos.y = m_current_pos.y = h / 2.0f;
            glfwSetCursorPos(m_window, m_current_pos.x, m_current_pos.y);
            break;
        }

        case Mode::IN_UI: {
            double x, y;
            glfwGetCursorPos(m_window, &x, &y);
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
    switch (mode) {
        case Mode::IN_GAME:
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            break;

        case Mode::IN_UI:
            break;

        case Mode::RELEASED:
            break;
    }
}

void MouseControl::setMode(Mode mode) {
    if (mode != m_mode) {
        _removeMode(m_mode);
        m_mode = mode;
        _initMode(m_mode);
    }
}

void MouseControl::update() {
    int focus = glfwGetWindowAttrib(m_window, GLFW_FOCUSED);
    if (m_focus != focus) {
        m_focus = focus;
        if (!m_focus) {
            _removeMode(m_mode);
            m_active = false;
        }
    }

    if (!m_active) {
        if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
            _initMode(m_mode);
            m_active = true;
        } else {
            return;
        }
    }

    m_last_pos = m_current_pos;

    double x, y;
    glfwGetCursorPos(m_window, &x, &y);
    m_current_pos.x = x;
    m_current_pos.y = y;

    switch (m_mode) {
        case Mode::IN_GAME: {
            int w, h;
            glfwGetWindowSize(m_window, &w, &h);
            glfwSetCursorPos(m_window, m_last_pos.x = w / 2.0f, m_last_pos.y = h / 2.0f);
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
    return glfwGetMouseButton(m_window, button);
}

} // input
} // voxel
