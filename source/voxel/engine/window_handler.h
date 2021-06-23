#ifndef VOXEL_ENGINE_WINDOW_HANDLER_H
#define VOXEL_ENGINE_WINDOW_HANDLER_H

#include <set>
#include <GLFW/glfw3.h>


namespace voxel {

    class IWindowHandlerListener {
    public:
        virtual void onWindowFocusLost();
        virtual void onWindowFocusGained();
        virtual void onWindowHandlerDestroyed();
    };

    class WindowHandler {
        GLFWwindow* m_window = nullptr;

        int m_window_focus = 1;
        bool m_focused = true;
        bool m_was_focused = true;

        // all listeners
        std::set<IWindowHandlerListener*> m_listeners;

    public:
        WindowHandler(GLFWwindow* window);
        WindowHandler(const WindowHandler& other) = delete;
        WindowHandler(WindowHandler&& other) = default;
        ~WindowHandler();

        void update();
        void addListener(IWindowHandlerListener* listener);
        void removeListener(IWindowHandlerListener* listener);

        GLFWwindow* getWindow();
        bool isFocused();
        bool wasFocused();
        int getWindowFocus();
    };


} // voxel

#endif //VOXEL_ENGINE_WINDOW_HANDLER_H
