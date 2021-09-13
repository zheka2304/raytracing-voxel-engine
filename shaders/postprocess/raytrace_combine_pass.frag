#version 420 core

#include "util.inc"
#include "common.inc"


uniform sampler2D IN_TEXTURE_COLOR;
uniform sampler2D IN_TEXTURE_LIGHT;
uniform sampler2D IN_TEXTURE_DEPTH;

varying vec2 uv;

float searchForClosestLight(vec2 uv, float sample_size) {
    int sample_size_i = 5;
    float step = 0.002f * sample_size;

    float z = texture(IN_TEXTURE_DEPTH, uv).r;
    float distance_sqr = sample_size * sample_size;

    float ww = 0.0;
    float vv = 0.0;
    for (int x = -sample_size_i; x <= sample_size_i; x++) {
        for (int y = -sample_size_i; y <= sample_size_i; y++) {
            if (x == 0 && y == 0) continue;
            vec2 uv_add = vec2(x, y);
            vec2 uv1 = uv + vec2(x, y) * step;
            float d = dot(uv_add, uv_add) + pow((texture(IN_TEXTURE_DEPTH, uv1).r - z) / step, 2.0);
            if (d < distance_sqr) {
                vec4 light_data = texture(IN_TEXTURE_LIGHT, uv1);
                float v = light_data.r >= POSITIVE_INF ? 0.0 : 1.0;
                float w = 1.0 - pow(d / distance_sqr, 0.5);
                vv += v * w;
                ww += w;
                distance_sqr = d;
            }
        }
    }
    return vv / ww;
}



float getBlurForLevel(vec3 blur_data, float level) {
    float p1 = unpackHalf2x16(floatBitsToUint(blur_data.r)).y;
    float p2 = unpackHalf2x16(floatBitsToUint(blur_data.g)).y;
    float p3 = unpackHalf2x16(floatBitsToUint(blur_data.b)).y;
    return clamp(p1 / level + 0.5 - 0.5 / level, 0.0, 1.0);
}

void main() {
    vec4 color_data = texture(IN_TEXTURE_COLOR, uv);
    vec4 light_data = texture(IN_TEXTURE_LIGHT, uv);
    vec4 depth_data = texture(IN_TEXTURE_DEPTH, uv);

    // extract colors
    ColorPair colors = unpackColorPairF(color_data.rgb);

    // extract all light-related data
    float distance_from_occluding_object = light_data.r;
    float distance_from_light_source = light_data.g;
    float light_size = light_data.b;
    float pixel_z = depth_data.r;

    float lambda_occlusion = distance_from_occluding_object / distance_from_light_source;
    float sample_size = (1.0 - lambda_occlusion) * light_size / pixel_z;
    float result_light_value = light_data.r;

    vec2 pair = unpackHalf2x16(floatBitsToUint(light_data.b));
    float shadow_value = light_data.g / 6.0 > 0.25 ? 1.0 : 0.0;

    vec4 result = color_data;
    result.rgb = mix(colors.color1.rgb, vec3(0.0), (1.0 - light_data.r) * 0.8);
    gl_FragColor = result;
    gl_FragColor.rgb = vec3(light_data.r);
}