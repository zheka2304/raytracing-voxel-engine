#ifndef VOXEL_ENGINE_LIGHTMAP_H
#define VOXEL_ENGINE_LIGHTMAP_H

#include "voxel/common/base.h"
#include "voxel/common/opengl.h"
#include "render_context.h"


namespace voxel {
namespace render {


class LightMapTexture {
    i32 m_width, m_height;

    opengl::Texture* m_interpolation_frame_last;
    opengl::Texture* m_interpolation_frame_current;
    opengl::Texture* m_blur_frame_last;
    opengl::Texture* m_blur_frame_current;

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
    void _bindTexturesForInterpolatePass(opengl::ShaderManager& shader_manager);
    void _bindTexturesForBlurPass(opengl::ShaderManager& shader_manager);
    void _swapInterpolationFrames();
    void _swapBlurFrames();
};


} // render
} // voxel

#endif //VOXEL_ENGINE_LIGHTMAP_H
