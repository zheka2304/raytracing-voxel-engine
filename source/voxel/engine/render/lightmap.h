#ifndef VOXEL_ENGINE_LIGHTMAP_H
#define VOXEL_ENGINE_LIGHTMAP_H

#include "voxel/common/base.h"
#include "voxel/common/opengl.h"
#include "render_context.h"


namespace voxel {
namespace render {


class LightMapTexture {
    i32 m_width, m_height;

    opengl::Texture* m_last_frame;
    opengl::Texture* m_current_frame;
    opengl::Texture m_blur_pass_texture;

public:
    LightMapTexture(i32 width, i32 height);
    LightMapTexture(const LightMapTexture& other) = delete;
    LightMapTexture(LightMapTexture&& other) = default;
    ~LightMapTexture();

    opengl::Texture& getLightmapTexture();
    opengl::Texture& getBlurPassTexture();

    void runInterpolationPass(RenderContext& render_context);
    void runBlurPass(RenderContext& render_context);

private:
    void _bindTexturesForLightmapPass(opengl::ShaderManager& shader_manager);
};


} // render
} // voxel

#endif //VOXEL_ENGINE_LIGHTMAP_H
