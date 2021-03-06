#include "render_context.h"

#include "voxel/engine/engine.h"


namespace voxel {
namespace render {

RenderContext::RenderContext(Engine& engine) :
        m_logger(engine.getLogger()),
        // TODO: shaders directory must be acquired from engine config
        m_shader_manager("shaders/", m_logger) {
}

RenderContext::~RenderContext() {
}


opengl::ShaderManager& RenderContext::getShaderManager() {
    return m_shader_manager;
}

void RenderContext::initialize() {
    m_logger.message(Logger::flag_debug, "RenderContext", "initializing render context");
    m_shader_manager.loadDeclarationsJson("presets.json");
    m_shader_manager.loadDeclarationsJson("engine.json");
}


} // render
} // voxel