#version 420 core

#include "util.inc"


uniform sampler2D IN_TEXTURE_COLOR;
uniform sampler2D IN_TEXTURE_LIGHT;
uniform sampler2D IN_TEXTURE_DEPTH;

varying vec2 uv;


#define PACK_H2F(X) uintBitsToFloat(packHalf2x16(X))
#define UNPACK_H2F(X) unpackHalf2x16(floatBitsToUint(X))
#define PACK_I2F(X) uintBitsToFloat(packInt2x16(X))
#define UNPACK_I2F(X) unpackInt2x16(floatBitsToUint(X))

void main() {
    vec4 color_data = texture(IN_TEXTURE_COLOR, uv);
    vec4 light_data = texture(IN_TEXTURE_LIGHT, uv);
    vec4 depth_data = texture(IN_TEXTURE_DEPTH, uv);

    // extract colors
    ColorPair colors = unpackColorPairF(color_data.rgb);

    // extract light value
    vec2 light_value_v2 = UNPACK_H2F(light_data.r);
    float light_value = mix(light_value_v2.x, light_value_v2.y, 1.0 - clamp(1.0 / (abs(light_value_v2.x - light_value_v2.y) * 100.0), 0.0, 1.0));

    vec4 result = color_data;
    result.rgb = mix(colors.color1.rgb, colors.color2.rgb, 1.0 - light_value);
    gl_FragColor = result;

    // -- DEBUG START --
//    gl_FragColor.rgb = vec3(color_data.rg, color_data.b);
//    gl_FragColor.rgb = vec3(light_value, light_value, color_data.b);
//    gl_FragColor.rgb = vec3(light_value, light_value, 0.0);
//    gl_FragColor.rgb = vec3(abs(color_data.rg), 0.0);
//    gl_FragColor.rgb = vec3(light_value);
//    gl_FragColor.rgb = vec3(depth_data.r);
    // -- DEBUG END --
}