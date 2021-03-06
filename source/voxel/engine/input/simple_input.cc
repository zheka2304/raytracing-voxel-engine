#include "simple_input.h"


namespace voxel {
namespace input {

SimpleInput::SimpleInput(WindowHandler& window_handler) :
    m_window_handler(window_handler), m_mouse_control(m_window_handler) {
}

MouseControl& SimpleInput::getMouseControl() {
    return m_mouse_control;
}

void SimpleInput::setMovementSpeed(f32 movement_speed) {
    m_movement_speed = movement_speed;
}

void SimpleInput::setSensitivity(f32 x, f32 y) {
    m_sensitivity.x = x;
    m_sensitivity.y = y;
}

void SimpleInput::update(render::Camera& camera) {
    m_mouse_control.update();

    if (m_window_handler.isFocused()) {
        // rotation
        voxel::math::Vec2f mouse_move = m_mouse_control.getMouseMove() * m_sensitivity;
        voxel::math::Vec3f& camera_rotation = camera.getProjection().rotation;
        camera_rotation.y += mouse_move.x;
        camera_rotation.x = std::max(-3.1416f / 2.0f, std::min(3.1416f / 2.0f, camera_rotation.x - mouse_move.y));

        // movement
        GLFWwindow* window = m_window_handler.getWindow();
        voxel::math::Vec3f& camera_position = camera.getProjection().position;
        if (glfwGetKey(window, GLFW_KEY_W)) {
            camera_position.x += cos(camera_rotation.x) * sin(camera_rotation.y) * m_movement_speed;
            camera_position.y += sin(camera_rotation.x) * m_movement_speed;
            camera_position.z += cos(camera_rotation.x) * cos(camera_rotation.y) * m_movement_speed;
        }
        if (glfwGetKey(window, GLFW_KEY_S)) {
            camera_position.x -= cos(camera_rotation.x) * sin(camera_rotation.y) * m_movement_speed;
            camera_position.y -= sin(camera_rotation.x) * m_movement_speed;
            camera_position.z -= cos(camera_rotation.x) * cos(camera_rotation.y) * m_movement_speed;
        }
        if (glfwGetKey(window, GLFW_KEY_D)) {
            camera_position.x += cos(camera_rotation.y) * m_movement_speed;
            camera_position.z -= sin(camera_rotation.y) * m_movement_speed;
        }
        if (glfwGetKey(window, GLFW_KEY_A)) {
            camera_position.x -= cos(camera_rotation.y) * m_movement_speed;
            camera_position.z += sin(camera_rotation.y) * m_movement_speed;
        }
    }
}

} // input
} // voxel