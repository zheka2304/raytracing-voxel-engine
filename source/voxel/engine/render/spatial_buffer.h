#ifndef VOXEL_ENGINE_SPATIAL_BUFFER_H
#define VOXEL_ENGINE_SPATIAL_BUFFER_H

#include "voxel/common/base.h"
#include "voxel/common/opengl.h"
#include "voxel/engine/render/render_context.h"


namespace voxel {
namespace render {

class SpatialRenderBuffer {
    i32 m_width, m_height;

    // last and current frame buffer textures, stores positions of all pixel in world space, swapped each frame
    opengl::Texture* m_last_frame;
    opengl::Texture* m_current_frame;

    // resulting spatial buffer
    //   Stores last screen space (pixel) positions and stability value of each pixel, if stability is 0,
    // no position might be stored.
    //   Stability is float value between 0 and 1, representing estimated value
    // of data integrity for this pixel. This value is starting at 0, increased each frame, when pixel is remaining visible,
    // and decreased by various filters, to prevent visual artifacts.
    //   Pixels with lower stability should be re-calculated more rapidly
    opengl::ShaderStorageBuffer m_spatial_buffer;
    opengl::ShaderStorageBuffer m_spatial_buffer_spinlock;

public:
    SpatialRenderBuffer(i32 width, i32 height);
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
