#ifndef VOXEL_ENGINE_COLOR_H
#define VOXEL_ENGINE_COLOR_H

#include "voxel/common/base.h"


namespace voxel {
namespace math {

class Color {
public:
    f32 r, g, b, a;

    Color(f32 r, f32 g, f32 b, f32 a = 1);
    Color(u32 hex, u32 color_bits = 8, u32 alpha_bits = 8);
    u32 toHex(u32 color_bits = 8, u32 alpha_bits = 8);
};

} // math
} // voxel

#endif //VOXEL_ENGINE_COLOR_H
