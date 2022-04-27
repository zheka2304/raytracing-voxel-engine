#version 420 core

#include "util.inc"


uniform sampler2D IN_TEXTURE_COLOR;
uniform sampler2D IN_TEXTURE_LIGHT;

in vec2 uv;
out vec4 out_color;

#define PACK_H2F(X) uintBitsToFloat(packHalf2x16(X))
#define UNPACK_H2F(X) unpackHalf2x16(floatBitsToUint(X))
#define PACK_I2F(X) uintBitsToFloat(packInt2x16(X))
#define UNPACK_I2F(X) unpackInt2x16(floatBitsToUint(X))


float getLightColorComponent(float packed_input) {
    vec2 light_value_v2 = UNPACK_H2F(packed_input);
    float light_value_denoise_factor = 1.0 - clamp(1.0 / (abs(light_value_v2.x - light_value_v2.y) / ${lighting.linear_blur_blend_factor} + 1.0), 0.0, 1.0);
    float light_value = mix(light_value_v2.x, light_value_v2.y, light_value_denoise_factor);
    return light_value;
}

void main() {
    vec4 color_data = texture(IN_TEXTURE_COLOR, uv);
    vec4 light_data = texture(IN_TEXTURE_LIGHT, uv);

    // extract colors
    ColorPair colors = unpackColorPairF(color_data.rgb);

    // extract light value
    vec3 light_value = vec3(getLightColorComponent(light_data.r), getLightColorComponent(light_data.g), getLightColorComponent(light_data.b));

    // write result
    vec4 result = vec4(1.0);
    result.rgb = mix(colors.color1.rgb, colors.color2.rgb, 1.0 - light_value);
    out_color = result;

    // -- DEBUG START --
//    out_color.rgb = colors.color1.rgb * light_value;
//    out_color.rgb = vec3(color_data.rg, color_data.b);
//    out_color.rgb = vec3(light_value, light_value, color_data.b);
//    out_color.rgb = vec3(light_value, color_data.b - 1.0, fract(color_data.b));
//    out_color.rgb = vec3(light_value, light_value, 0.0);
//    out_color.rgb = vec3(abs(color_data.rg), 0.0);
//    out_color.rgb = vec3(light_value);
//    out_color.rgb = vec3(depth_data.r);
//    out_color.rgb = vec3(UNPACK_H2F(light_data.g) / 5.0, 0.0);
    // -- DEBUG END --
}