#include "render_context.h"

#include "voxel/engine/engine.h"


namespace voxel {
namespace render {

RenderContext::RenderContext(std::weak_ptr<Engine> engine) :
        m_logger(engine.lock()->getLogger()),
        // TODO: shaders directory must be acquired from engine config
        m_shader_manager("shaders/", m_logger) {
    m_shader_manager.loadDeclarationsJson("engine.json");
}

RenderContext::~RenderContext() {
}

opengl::ShaderManager& RenderContext::getShaderManager() {
    return m_shader_manager;
}

} // render
} // voxel