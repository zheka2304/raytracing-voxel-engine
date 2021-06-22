#ifndef VOXEL_ENGINE_MOUSE_H
#define VOXEL_ENGINE_MOUSE_H

#include <glfw/glfw3.h>

#include "voxel/common/math/vec.h"


namespace voxel {
namespace input {

class MouseControl {
public:
    enum class Mode {
        RELEASED,
        IN_GAME,
        IN_UI
    };

private:
    GLFWwindow* m_window;

    Mode m_mode = Mode::RELEASED;
    int m_focus = 1;
    bool m_active = true;

    math::Vec2 m_last_pos;
    math::Vec2 m_current_pos;

    void _initMode(Mode mode);
    void _removeMode(Mode mode);
public:
    MouseControl(GLFWwindow* window);
    MouseControl(const MouseControl& other) = delete;
    MouseControl(MouseControl&& other) = default;
    ~MouseControl();

    void setMode(Mode mode);
    void update();

    math::Vec2 getLastPos();
    math::Vec2 getMousePos();
    math::Vec2 getMouseMove();
    int getMouseButton(int button);
};

} // input
} // voxel

#endif //VOXEL_ENGINE_MOUSE_H
