#ifndef VOXEL_ENGINE_RENDER_CONTEXT_H
#define VOXEL_ENGINE_RENDER_CONTEXT_H

#include <glad/glad.h>
#include "voxel/common/base.h"
#include "voxel/common/logger.h"
#include "voxel/common/opengl.h"


namespace voxel {

class Engine;

namespace render {

class RenderContext {
    Logger m_logger;
    opengl::ShaderManager m_shader_manager;

public:
    RenderContext(Weak<Engine> engine);
    RenderContext(RenderContext const& other) = delete;
    RenderContext(RenderContext&& other) = delete;
    ~RenderContext();

    void initialize();

    opengl::ShaderManager& getShaderManager();
};

} // render
} // voxel

#endif //VOXEL_ENGINE_RENDER_CONTEXT_H
