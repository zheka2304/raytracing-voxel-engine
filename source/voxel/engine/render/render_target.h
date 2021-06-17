#ifndef VOXEL_ENGINE_RENDER_TARGET_H
#define VOXEL_ENGINE_RENDER_TARGET_H

#include "voxel/common/opengl.h"
#include "voxel/engine/render/render_context.h"


namespace voxel {
namespace render {

class RenderTarget {
    opengl::Texture m_color_texture;
    opengl::Texture m_light_texture;
    opengl::Texture m_depth_texture;

public:
    void bind(RenderContext& render_context);
};

} // render
} // voxel

#endif //VOXEL_ENGINE_RENDER_TARGET_H
