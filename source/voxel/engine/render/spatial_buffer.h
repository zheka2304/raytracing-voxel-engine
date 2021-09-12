#ifndef VOXEL_ENGINE_SPATIAL_BUFFER_H
#define VOXEL_ENGINE_SPATIAL_BUFFER_H

#include "voxel/common/opengl.h"
#include "voxel/engine/render/render_context.h"


namespace voxel {
namespace render {

class SpatialRenderBuffer {
    int m_width, m_height;

    // last and current frame buffer textures, stores positions of all pixel in world space
    // swapped each frame
    opengl::Texture* m_last_frame;
    opengl::Texture* m_current_frame;

    // resulting spatial buffer
    // stores last screen space positions and temporal stability value of each pixel, if temporal stability is 0, no position is stored
    opengl::Texture m_spatial_buffer;

public:
    SpatialRenderBuffer(int width, int height);
    SpatialRenderBuffer(const SpatialRenderBuffer& other) = delete;
    SpatialRenderBuffer(SpatialRenderBuffer&& other) = delete;
    ~SpatialRenderBuffer();

    void bind(RenderContext& context);

    // after current frame is filled, swap the frame and update spatial buffer
    void runSwap(RenderContext& context);
};

} // render
} // voxel

#endif //VOXEL_ENGINE_SPATIAL_BUFFER_H
