#include "color.h"


namespace voxel {
namespace math {

Color::Color(f32 r, f32 g, f32 b, f32 a) : r(r), g(g), b(b), a(a) {
}

Color::Color(u32 hex, u32 color_bits, u32 alpha_bits) {
    u32 color_mask = (1u << color_bits) - 1u;
    u32 alpha_mask = (1u << alpha_bits) - 1u;
    r = (hex & color_mask) / float(color_mask); hex >>= color_bits;
    g = (hex & color_mask) / float(color_mask); hex >>= color_bits;
    b = (hex & color_mask) / float(color_mask); hex >>= color_bits;
    a = (hex & alpha_mask) / float(alpha_mask);
}

u32 Color::toHex(u32 color_bits, u32 alpha_bits) {
    u32 color_mask = (1u << color_bits) - 1u;
    u32 alpha_mask = (1u << alpha_bits) - 1u;
    u32 r_i = u8(r * color_mask) & color_mask;
    u32 g_i = u8(g * color_mask) & color_mask;
    u32 b_i = u8(b * color_mask) & color_mask;
    u32 a_i = u8(a * alpha_mask) & color_mask;
    return r_i | ((g_i | ((b_i | (a_i << color_bits)) << color_bits)) << color_bits);
}

} // math
} // voxel