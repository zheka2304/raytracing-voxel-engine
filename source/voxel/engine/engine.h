#ifndef VOXEL_ENGINE_ENGINE_H
#define VOXEL_ENGINE_ENGINE_H

#include <memory>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

#include <GLFW/glfw3.h>

#include "voxel/common/utils/worker_thread.h"
#include "voxel/common/logger.h"
#include "voxel/engine/window_handler.h"
#include "voxel/engine/render/render_context.h"


namespace voxel {

// pre-declare all classes
class Engine;
class Context;
class World;


// root class of voxel engine, runs base initialization, allows creation and handling of contexts
class Engine : public std::enable_shared_from_this<Engine> {
    // contains initialization result, if everything is ok, it is 0
    // otherwise if glfw init failed, it will be -1, or, in other case - glad error code
    int m_initialization_result;

    // main logger of the engine, it is used by other modules
    Logger m_logger;

    // list of all spawned contexts
    std::vector<std::shared_ptr<Context>> m_contexts;

    // thread for initializing glfw contexts and other glfw-related work
    utils::WorkerThread m_glfw_thread;

public:
    Engine();
    Engine(const Engine& other) = delete;
    Engine(Engine&& other) = delete;
    ~Engine();

    // initializes glfw and glad, returns initialization result (0 on success)
    int initialize();
    int getInitializationResult();

    // get thread to execute all glfw initialization
    utils::WorkerThread& getGlfwThread();

    // getters
    Logger& getLogger();

    // creates and initializes new context from this engine
    std::shared_ptr<Context> newContext(const std::string& context_name);

    // joins all active contexts event loops, consider using this in main after doing all initialization and starting all event loops
    void joinAllEventLoops();
};

// represents one window and handles all rendering & input-related actions
class Context {
public:
    struct WindowParameters {
        int width;
        int height;
        const char* title;
        std::vector<std::pair<int, int>> hints;
    };

private:
    std::string m_context_name;
    std::weak_ptr<Engine> m_engine;
    std::shared_ptr<World> m_world;
    std::shared_ptr<render::RenderContext> m_render_context;

    Logger m_logger;

    // GLFW window (in GLFW acts like context)
    GLFWwindow* m_window = nullptr;

    // window handler
    WindowHandler* m_window_handler = nullptr;

    // true, if window was initialized via initWindow(), in case of initNoWindow() or no init call it is false
    bool m_has_window = false;

    // GLFW window must be created on the thread it will run on, so initWindow and initNoWindow queues their logic via this function
    std::function<void()> m_window_initializer;

    // event loop
    std::mutex m_event_loop_mutex;
    std::thread m_event_loop_thread;
    std::atomic_bool m_termination_pending = false;
    std::atomic_bool m_event_loop_running = false;
    std::condition_variable m_event_loop_start_notifier;

    // window callbacks
    std::function<void(Context&, render::RenderContext&)> m_init_callback;
    std::function<void(Context&, render::RenderContext&)> m_frame_handle_callback;
    std::function<void(Context&)> m_event_process_callback;
    std::function<void(Context&, int, int)> m_window_resize_callback;
    std::function<void(Context&, int)> m_window_focus_callback;
    std::function<void(Context&)> m_destroy_callback;

public:
    Context(std::weak_ptr<Engine> engine, const std::string& context_name);
    Context(const Context& other) = delete;
    Context(Context&& other) = delete;
    ~Context();

    // getters
    std::string getContextName();
    Engine& getEngine();
    Logger& getLogger();
    GLFWwindow* getGlfwWindow();
    GLFWwindow* awaitGlfwWindow();
    WindowHandler& getWindowHandler();

    void setInitCallback(const std::function<void(Context&, render::RenderContext&)>& callback);
    void setFrameHandleCallback(const std::function<void(Context&, render::RenderContext&)>& callback);
    void setEventProcessingCallback(const std::function<void(Context&)>& callback);
    void setWindowResizeCallback(const std::function<void(Context&, int, int)>& callback);
    void setWindowFocusCallback(const std::function<void(Context&, int)>& callback);
    void setDestroyCallback(const std::function<void(Context&)>& callback);

    // creates and initializes window with given parameters
    void initWindow(WindowParameters parameters, std::shared_ptr<Context> shared_context = nullptr);

    // initializes context without window (invisible window is used, because in GLFW window = context)
    void initNoWindow(std::shared_ptr<Context> shared_context = nullptr);

    // starts window event loop
    void runEventLoop();

    // terminates event loop and joins it, then destroys GLFW context
    void terminateEventLoop();

    // joins context event loop thread, if it is running
    // Engine::joinAllEventLoops() should be used instead of this
    void joinEventLoop();

private:
    bool initializeGlad();
    void initializeRenderContext(std::shared_ptr<Context> shared_context);

    void eventLoop();
    void processEvents();
    void handleFrame();

    static void glfwWindowSizeCallback(GLFWwindow* window, int width, int height);
    static void glfwWindowFocusCallback(GLFWwindow* window, int focus);
};

} // voxel

#endif //VOXEL_ENGINE_ENGINE_H
