#ifndef VOXEL_ENGINE_SIMPLE_INPUT_H
#define VOXEL_ENGINE_SIMPLE_INPUT_H

#include "voxel/common/base.h"
#include "voxel/common/math/vec.h"
#include "voxel/engine/window_handler.h"
#include "voxel/engine/input/mouse.h"
#include "voxel/engine/render/camera.h"


namespace voxel {
namespace input {

class SimpleInput {
    WindowHandler& m_window_handler;
    MouseControl m_mouse_control;

    f32 m_movement_speed = 0.1f;
    math::Vec2f m_sensitivity = math::Vec2f(0.01f, 0.01f);

public:
    SimpleInput(WindowHandler& window_handler);
    SimpleInput(const SimpleInput& other) = delete;
    SimpleInput(SimpleInput&& other) = default;

    MouseControl& getMouseControl();
    void setMovementSpeed(f32 movement_speed);
    void setSensitivity(f32 x, f32 y);
    void update(render::Camera& camera);
};

} // input
} // voxel

#endif //VOXEL_ENGINE_SIMPLE_INPUT_H
