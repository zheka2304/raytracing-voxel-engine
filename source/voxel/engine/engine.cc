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
        if (!glfwInit()) {
            m_logger.message(Logger::flag_error | Logger::flag_critical, "Engine", "GLFW init failed");
            return m_initialization_result = -1;
        } else {
            m_logger.message(Logger::flag_info, "Engine", "engine successfully initialized");
            return m_initialization_result = 0;
        }
    }

    int Engine::getInitializationResult() {
        return m_initialization_result;
    }

    Logger& Engine::getLogger() {
        return m_logger;
    }

    std::shared_ptr<Context> Engine::newContext() {
        std::shared_ptr<Context> context = std::make_shared<Context>(weak_from_this());
        m_contexts.emplace_back(context);
        return context;
    }

    void Engine::joinAllEventLoops() {
        for (std::shared_ptr<Context>& ctx : m_contexts) {
            ctx->joinEventLoop();
        }
    }


    Context::Context(std::weak_ptr<Engine> engine) :
        m_engine(engine), m_logger(engine.lock()->getLogger()), m_event_loop_thread(&Context::eventLoop, this) {
    }

    Context::~Context() {
        terminateEventLoop();
    }

    GLFWwindow* Context::getGlfwWindow() {
        return m_window;
    }

    void Context::initWindow(WindowParameters parameters, std::shared_ptr<Context> shared_context) {
        std::unique_lock<std::mutex> lock(m_event_loop_mutex);

        if (m_termination_pending) {
            m_logger.message(Logger::flag_error, "Context", "failed to call initWindow(): termination pending");
            return;
        }

        if (m_window_initializer != nullptr) {
            m_logger.message(Logger::flag_error, "Context", "failed to call initWindow(): init was already called");
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
                    shared_context.get() != nullptr ? shared_context->getGlfwWindow() : nullptr);

            // initialized with window
            m_has_window = true;
        };
    }

    void Context::initNoWindow(std::shared_ptr<Context> shared_context) {
        std::unique_lock<std::mutex> lock(m_event_loop_mutex);

        if (m_termination_pending) {
            m_logger.message(Logger::flag_error, "Context", "failed to call initNoWindow(): termination pending");
            return;
        }

        if (m_window_initializer != nullptr) {
            m_logger.message(Logger::flag_error, "Context", "failed to call initNoWindow(): init was already called");
            return;
        }

        m_window_initializer = [=] () -> void {
            // create invisible window
            glfwDefaultWindowHints();
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
            m_window = glfwCreateWindow(0, 0, "", nullptr,
                                        shared_context.get() != nullptr ? shared_context->getGlfwWindow() : nullptr);

            // initialized without window
            m_has_window = false;
        };
    }

    void Context::runEventLoop() {
        std::unique_lock<std::mutex> lock(m_event_loop_mutex);
        if (!m_event_loop_running && !m_termination_pending) {
            m_event_loop_running = true;
            m_event_loop_start_notifier.notify_one();
        }
    }

    void Context::terminateEventLoop() {
        std::unique_lock<std::mutex> lock(m_event_loop_mutex);
        if (!m_termination_pending) {
            m_termination_pending = true;
            m_event_loop_start_notifier.notify_one();
            m_event_loop_thread.join();
        }
    }

    void Context::joinEventLoop() {
        if (m_event_loop_running) {
            m_event_loop_thread.join();
        }
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
            m_logger.message(Logger::flag_error | Logger::flag_critical, "Context", "context started event loop without calling initWindow or initNoWindow, it will be terminated");
            m_termination_pending = true;
            return;
        }

        // initialize GLAD
        glfwMakeContextCurrent(m_window);
        int glad_init_result = gladLoadGL();
        if (!glad_init_result) {
            m_logger.message(Logger::flag_error | Logger::flag_critical, "Context", "failed to initialize GLAD");
            m_termination_pending = true;
            glfwDestroyWindow(m_window);
            m_window = nullptr;
            return;
        }

        // set window parameters and unlock
        glfwSwapInterval(1);
        glfwShowWindow(m_window);
        lock.unlock();

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

        // destroy window at the end of the loop
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    void Context::processEvents() {

    }

    void Context::handleFrame() {
        m_logger.message(Logger::flag_debug, "Context", "+");
    }

} // voxel