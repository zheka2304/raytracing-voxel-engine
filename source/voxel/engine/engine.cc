#include "engine.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>


namespace voxel {
    Engine::Engine() {

    }

    Engine::~Engine() {
        m_logger.message(Logger::flag_info, "Engine", "terminating all contexts");
        for (auto& ctx : m_contexts) {
            ctx->terminateEventLoop();
        }

        if (m_initialization_result == 0) {
            m_logger.message(Logger::flag_info, "Engine", "destroying engine, terminating GLFW");
            glfwTerminate();
        } else {
            m_logger.message(Logger::flag_info, "Engine", "destroying uninitialized engine");
        }
    }

    int Engine::initialize() {
        return m_glfw_thread.awaitResult<int>([=] () -> int {
            if (!glfwInit()) {
                m_logger.message(Logger::flag_error | Logger::flag_critical, "Engine", "GLFW init failed");
                return m_initialization_result = -1;
            } else {
                m_logger.message(Logger::flag_info, "Engine", "engine successfully initialized");
                return m_initialization_result = 0;
            }
        });
    }

    utils::WorkerThread & Engine::getGlfwThread() {
        return m_glfw_thread;
    }

    int Engine::getInitializationResult() {
        return m_initialization_result;
    }

    Logger& Engine::getLogger() {
        return m_logger;
    }

    std::shared_ptr<Context> Engine::newContext(const std::string& context_name) {
        std::shared_ptr<Context> context = std::make_shared<Context>(weak_from_this(), context_name);
        m_contexts.emplace_back(context);
        return context;
    }

    void Engine::joinAllEventLoops() {
        for (std::shared_ptr<Context>& ctx : m_contexts) {
            ctx->joinEventLoop();
        }
    }


    Context::Context(std::weak_ptr<Engine> engine, const std::string& context_name) :
            m_context_name(context_name),
            m_engine(engine),
            m_logger(engine.lock()->getLogger()),
            m_event_loop_thread(&Context::eventLoop, this) {
    }

    Context::~Context() {
        terminateEventLoop();
    }

    std::string Context::getContextName() {
        return m_context_name;
    }

    Engine& Context::getEngine() {
        return *m_engine.lock().get();
    }

    Logger& Context::getLogger() {
        return m_logger;
    }

    GLFWwindow* Context::getGlfwWindow() {
        return m_window;
    }

    GLFWwindow* Context::awaitGlfwWindow() {
        std::unique_lock<std::mutex> lock(m_event_loop_mutex);
        m_event_loop_start_notifier.wait(lock, [=] () -> bool {
            return m_window != nullptr;
        });
        return m_window;
    }

    void Context::setInitCallback(const std::function<void(Context&, render::RenderContext&)>& callback) {
        m_init_callback = callback;
    }

    void Context::setFrameHandleCallback(const std::function<void(Context&, render::RenderContext&)>& callback) {
        m_frame_handle_callback = callback;
    }

    void Context::setEventProcessingCallback(const std::function<void(Context&)>& callback) {
        m_event_process_callback = callback;
    }

    void Context::setWindowResizeCallback(const std::function<void(Context&, int, int)>& callback) {
        m_window_resize_callback = callback;
    }

    void Context::setWindowFocusCallback(const std::function<void(Context&, int)>& callback) {
        m_window_focus_callback = callback;
    }

    void Context::setDestroyCallback(const std::function<void(Context&)>& callback) {
        m_destroy_callback = callback;
    }

    void Context::initWindow(WindowParameters parameters, std::shared_ptr<Context> shared_context) {
        std::unique_lock<std::mutex> lock(m_event_loop_mutex);

        if (m_termination_pending) {
            m_logger.message(Logger::flag_error, "Context-" + m_context_name, "failed to call initWindow(): termination pending");
            return;
        }

        if (m_window_initializer != nullptr) {
            m_logger.message(Logger::flag_error, "Context-" + m_context_name, "failed to call initWindow(): init was already called");
            return;
        }

        m_window_initializer = [=] () -> void {
            glfwDefaultWindowHints();
            for (auto& hint : parameters.hints) {
                glfwWindowHint(hint.first, hint.second);
            }

            m_window = glfwCreateWindow(
                    parameters.width,
                    parameters.height,
                    parameters.title,
                    nullptr,
                    shared_context.get() != nullptr ? shared_context->awaitGlfwWindow() : nullptr);

            // initialized with window
            m_has_window = true;

            initializeRenderContext(shared_context);
        };
    }

    void Context::initNoWindow(std::shared_ptr<Context> shared_context) {
        std::unique_lock<std::mutex> lock(m_event_loop_mutex);

        if (m_termination_pending) {
            m_logger.message(Logger::flag_error, "Context-" + m_context_name, "failed to call initNoWindow(): termination pending");
            return;
        }

        if (m_window_initializer != nullptr) {
            m_logger.message(Logger::flag_error, "Context-" + m_context_name, "failed to call initNoWindow(): init was already called");
            return;
        }

        m_window_initializer = [=] () -> void {
            // create invisible window
            glfwDefaultWindowHints();
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

            m_window = glfwCreateWindow(640, 480, "hidden", nullptr,
                                        shared_context.get() != nullptr ? shared_context->awaitGlfwWindow() : nullptr);

            // initialized without window
            m_has_window = false;

            initializeRenderContext(shared_context);
        };
    }

    void Context::initializeRenderContext(std::shared_ptr<Context> shared_context) {
        if (m_render_context != nullptr) {
            m_logger.message(Logger::flag_error, "Context-" + m_context_name, "render context is already initialized");
            return;
        }

        if (shared_context != nullptr) {
            m_render_context = shared_context->m_render_context;
            if (m_render_context == nullptr) {
                m_logger.message(Logger::flag_error, "Context-" + m_context_name, "shared context is not initialized, separate render context will be created");
            }
        }

        if (m_render_context == nullptr) {
            m_render_context = std::make_shared<render::RenderContext>(m_engine);
        }
    }


    void Context::runEventLoop() {
        std::unique_lock<std::mutex> lock(m_event_loop_mutex);
        if (!m_event_loop_running && !m_termination_pending) {
            m_event_loop_running = true;
            m_event_loop_start_notifier.notify_all();
        }
    }

    void Context::terminateEventLoop() {
        std::unique_lock<std::mutex> lock(m_event_loop_mutex);
        if (!m_termination_pending) {
            m_termination_pending = true;
            m_event_loop_start_notifier.notify_all();
            m_event_loop_thread.join();
        }
    }

    void Context::joinEventLoop() {
        if (m_event_loop_running) {
            m_event_loop_thread.join();
        }
    }

    bool Context::initializeGlad() {
        static std::mutex glad_init_mutex;
        std::unique_lock<std::mutex> lock(glad_init_mutex);
        return gladLoadGL();
    }

    void Context::eventLoop() {
        // lock during checks and initialization
        std::unique_lock<std::mutex> lock(m_event_loop_mutex);
        m_event_loop_start_notifier.wait(lock, [=] () -> bool {
            return m_event_loop_running || m_termination_pending;
        });

        // if context was terminated, return
        if (m_termination_pending) {
            return;
        }

        // run window initializer, if it was empty, initialization method was not called
        m_window_initializer();
        if (m_window == nullptr) {
            m_logger.message(Logger::flag_error | Logger::flag_critical, "Context-" + m_context_name, "context started event loop without calling initWindow or initNoWindow, it will be terminated");
            m_termination_pending = true;
            return;
        }

        // initialize GLAD
        glfwMakeContextCurrent(m_window);
        if (!initializeGlad()) {
            m_logger.message(Logger::flag_error | Logger::flag_critical, "Context-" + m_context_name, "failed to initialize GLAD");
            m_termination_pending = true;
            glfwDestroyWindow(m_window);
            m_window = nullptr;
            return;
        }

        // initialize render context
        m_render_context->initialize();

        // set window parameters and unlock
        glfwSwapInterval(1);
        glfwShowWindow(m_window);

        // set window callbacks
        glfwSetWindowUserPointer(m_window, this);
        if (m_has_window) {
            glfwSetWindowSizeCallback(m_window, glfwWindowSizeCallback);
            glfwSetWindowFocusCallback(m_window, glfwWindowFocusCallback);
        }

        // invoke initialization callback first
        if (m_init_callback) {
            m_init_callback(*this, *m_render_context);
        }

        // then invoke size callback with initial size
        if (m_window_resize_callback) {
            int width, height;
            glfwGetWindowSize(m_window, &width, &height);
            m_window_resize_callback(*this, width, height);
        }

        // then invoke focus callback with focused state (window will always start focused)
        if (m_window_focus_callback) {
            m_window_focus_callback(*this, 1);
        }

        // unlock event loop mutex
        lock.unlock();

        m_event_loop_start_notifier.notify_all();

        // run main event loop
        while (!m_termination_pending) {
            // lock for executing each frame
            m_event_loop_mutex.lock();

            // check if window was closed
            if (glfwWindowShouldClose(m_window)) {
                m_termination_pending = true;
                m_event_loop_mutex.unlock();
                break;
            }

            // do all required stuff
            processEvents();
            handleFrame();

            // complete frame: swap buffers and poll events
            glfwSwapBuffers(m_window);
            glfwPollEvents();

            // unlock at the end of the frame
            m_event_loop_mutex.unlock();
        }

        // call destroy callback
        if (m_destroy_callback) {
            m_destroy_callback(*this);
        }

        // destroy window at the end of the loop
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    void Context::processEvents() {
        if (m_event_process_callback) {
            m_event_process_callback(*this);
        }
    }

    void Context::handleFrame() {
        glClearColor(1.0, 0.0, 1.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (m_frame_handle_callback) {
            m_frame_handle_callback(*this, *m_render_context);
        }
    }

    void Context::glfwWindowSizeCallback(GLFWwindow* window, int width, int height) {
        Context* context = static_cast<Context*>(glfwGetWindowUserPointer(window));
        if (context->m_window_resize_callback) {
            context->m_window_resize_callback(*context, width, height);
        }
    }

    void Context::glfwWindowFocusCallback(GLFWwindow* window, int focus) {
        Context* context = static_cast<Context*>(glfwGetWindowUserPointer(window));
        if (context->m_window_focus_callback) {
            context->m_window_focus_callback(*context, focus);
        }
    }

} // voxel