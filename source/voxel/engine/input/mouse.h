#ifndef VOXEL_ENGINE_MOUSE_H
#define VOXEL_ENGINE_MOUSE_H

#include <glfw/glfw3.h>

#include "voxel/common/base.h"
#include "voxel/common/math/vec.h"
#include "voxel/engine/window_handler.h"


namespace voxel {
namespace input {

class MouseControl : IWindowHandlerListener {
public:
    enum class Mode {
        RELEASED,
        IN_GAME,
        IN_UI
    };

private:
    WindowHandler* m_window_handler;

    Mode m_mode = Mode::RELEASED;
    i32 m_focus = 1;
    bool m_active = true;

    math::Vec2f m_last_pos;
    math::Vec2f m_current_pos;

    void _initMode(Mode mode);
    void _removeMode(Mode mode);

    void onWindowFocusGained() override;
    void onWindowFocusLost() override;
    void onWindowHandlerDestroyed() override;

public:
    explicit MouseControl(WindowHandler& window_handler);
    MouseControl(const MouseControl& other) = delete;
    MouseControl(MouseControl&& other) = default;
    ~MouseControl();

    void setMode(Mode mode);
    void update();

    math::Vec2f getLastPos();
    math::Vec2f getMousePos();
    math::Vec2f getMouseMove();
    i32 getMouseButton(i32 button);
};

} // input
} // voxel

#endif //VOXEL_ENGINE_MOUSE_H
