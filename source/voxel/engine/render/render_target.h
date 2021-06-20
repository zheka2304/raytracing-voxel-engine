#ifndef VOXEL_ENGINE_RENDER_TARGET_H
#define VOXEL_ENGINE_RENDER_TARGET_H

#include "voxel/common/math/vec.h"
#include "voxel/common/opengl.h"
#include "voxel/engine/render/render_context.h"


namespace voxel {
namespace render {

class RenderTarget {
    int m_width, m_height;

    opengl::Texture m_color_texture;
    opengl::Texture m_light_texture;
    opengl::Texture m_depth_texture;

    opengl::RenderToTexture m_result_render_to_texture;

public:
    RenderTarget(int width, int height);
    RenderTarget(const RenderTarget& other) = delete;
    RenderTarget(RenderTarget&& other) = default;

    // bind render target as a compute shader output
    // shader constant bindings:
    // - ${raytrace.color_texture}
    // - ${raytrace.light_texture}
    // - ${raytrace.depth_texture}
    void bindForCompute(RenderContext& render_context);

    // bind render target as a post processing shader input
    // shader uniform bindings and indices:
    // - IN_TEXTURE_COLOR (1)
    // - IN_TEXTURE_LIGHT (2)
    // - IN_TEXTURE_DEPTH (3)
    void bindForPostProcessing(opengl::GraphicsShader& post_processing_shader);

    int getWidth();
    int getHeight();
    math::Vec3i getComputeDispatchSize(int compute_group_size);

    opengl::Texture& getColorTexture();
    opengl::Texture& getLightTexture();
    opengl::Texture& getDepthTexture();

    opengl::RenderToTexture& getRenderToTexture();
    opengl::Texture& getResultTexture();
};

} // render
} // voxel

#endif //VOXEL_ENGINE_RENDER_TARGET_H
