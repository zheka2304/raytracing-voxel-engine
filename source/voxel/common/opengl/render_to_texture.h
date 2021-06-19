#ifndef VOXEL_ENGINE_RENDER_TO_TEXTURE_H
#define VOXEL_ENGINE_RENDER_TO_TEXTURE_H

#include <glad/glad.h>

#include "voxel/common/math/vec.h"
#include "voxel/common/math/color.h"
#include "voxel/common/opengl/mesh.h"
#include "voxel/common/opengl/texture.h"


namespace voxel {
namespace opengl {

class FullScreenQuad {
    struct VertexFormat {
        math::Vec3 position;
        math::Vec2 uv;

        inline static std::vector<VertexField> fields() {
            return {
                    {GL_FLOAT, 0, 3}, // 0) vec3 position
                    {GL_FLOAT, 3, 2}, // 1) vec2 uv
            };
        }
    };

    Mesh<VertexFormat> m_mesh;

public:
    FullScreenQuad(float z = 0);
    FullScreenQuad(const FullScreenQuad& other) = default;
    FullScreenQuad(FullScreenQuad&& other) = default;
    void render();
};

class RenderToTexture {
    struct Viewport {
        int x, y, w, h;
    };

    int m_width, m_height;
    math::Color m_background_color = math::Color(1, 1, 1, 1);
    std::vector<Texture> m_output_textures;

    GLuint m_framebuffer_handle = 0;
    GLuint m_depth_buffer_handle = 0;

    bool m_is_rendering = false;
    Viewport m_fallback_viewport;
    FullScreenQuad m_fullscreen_quad;

public:
    RenderToTexture(int width, int height, int output_texture_count = 1);
    RenderToTexture(const RenderToTexture&) = delete;
    RenderToTexture(RenderToTexture&&);
    ~RenderToTexture();

    // starts render to texture section
    void begin();

    // ends render to texture section
    void end();

    // runs complete fullscreen render sequence:
    // begin(); m_fullscreen_quad.render(); end();
    void render();


    // returns output texture ref with index
    Texture& getOutputTexture(int index = 0);

    // returns all output textures as vector
    std::vector<Texture>& getOutputTextures();
};

} // opengl
} // voxel

#endif //VOXEL_ENGINE_RENDER_TO_TEXTURE_H
