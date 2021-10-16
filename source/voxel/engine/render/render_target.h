#ifndef VOXEL_ENGINE_RENDER_TARGET_H
#define VOXEL_ENGINE_RENDER_TARGET_H

#include "voxel/common/base.h"
#include "voxel/common/math/vec.h"
#include "voxel/common/opengl.h"
#include "voxel/engine/render/render_context.h"
#include "voxel/engine/render/lightmap.h"
#include "voxel/engine/render/spatial_buffer.h"


namespace voxel {
namespace render {

class RenderTarget {
    i32 m_width, m_height;

    opengl::Texture m_color_texture;
    LightMapTexture m_lightmap;
    SpatialRenderBuffer m_spatial_buffer;

    opengl::RenderToTexture m_result_render_to_texture;

public:
    RenderTarget(i32 width, i32 height);
    RenderTarget(const RenderTarget& other) = delete;
    RenderTarget(RenderTarget&& other) = delete;

    // bind render target as a compute shader output
    // shader constant bindings:
    // - ${raytrace.color_texture}
    // - ${raytrace.light_texture}
    void bindForCompute(RenderContext& render_context);

    // bind render target as a post processing shader input
    // shader uniform bindings and indices:
    // - IN_TEXTURE_COLOR (1)
    // - IN_TEXTURE_LIGHT (2)
    void bindForPostProcessing(opengl::GraphicsShader& post_processing_shader);

    i32 getWidth();
    i32 getHeight();

    opengl::Texture& getColorTexture();
    LightMapTexture& getLightmap();
    SpatialRenderBuffer& getSpatialBuffer();

    opengl::RenderToTexture& getRenderToTexture();
    opengl::Texture& getResultTexture();
};

} // render
} // voxel

#endif //VOXEL_ENGINE_RENDER_TARGET_H
