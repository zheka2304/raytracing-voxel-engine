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

    vec4 result = color_data;
    result.rgb = mix(colors.color1.rgb, colors.color2.rgb, 1.0 - light_data.r);
    gl_FragColor = result;
//    gl_FragColor.rgb = vec3(color_data.rg, color_data.b);
    gl_FragColor.rgb = vec3(light_data.r, light_data.r, color_data.b);
//    gl_FragColor.rgb = vec3(abs(color_data.rg), 0.0);
//    gl_FragColor.rgb = vec3(light_data.r);
//    gl_FragColor.rgb = vec3(depth_data.r);
}