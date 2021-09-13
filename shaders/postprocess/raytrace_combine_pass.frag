#version 420 core

#include "util.inc"
#include "common.inc"


uniform sampler2D IN_TEXTURE_COLOR;
uniform sampler2D IN_TEXTURE_LIGHT;
uniform sampler2D IN_TEXTURE_DEPTH;

varying vec2 uv;


void main() {
    vec4 color_data = texture(IN_TEXTURE_COLOR, uv);
    vec4 light_data = texture(IN_TEXTURE_LIGHT, uv);
    vec4 depth_data = texture(IN_TEXTURE_DEPTH, uv);

    // extract colors
    ColorPair colors = unpackColorPairF(color_data.rgb);

    vec2 pair = unpackHalf2x16(floatBitsToUint(light_data.b));
    float shadow_value = light_data.g / 6.0 > 0.25 ? 1.0 : 0.0;

    vec4 result = color_data;
    result.rgb = mix(colors.color1.rgb, colors.color2.rgb, 1.0 - light_data.r);
    gl_FragColor = result;
    // gl_FragColor.rgb = vec3(light_data.r);
}