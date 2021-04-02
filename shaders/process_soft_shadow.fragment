#version 330 core

uniform sampler2D TEXTURE_0; // color
uniform sampler2D TEXTURE_1; // light
uniform sampler2D TEXTURE_2; // lowp depth

uniform vec2 BLEND_RADIUS;

varying vec4 color;
varying vec2 uv;

#define MUL_0 1.0
#define MUL_1 1.0
#define MUL_2 1.0

#define ADD_TO_BLEND0(LIGHT_VAR, SUM_VAR, F_VAR, BASE_DEPTH, UV_DELTA, FACTOR, NEXT_SAMPLE) \
    { \
        float F_VAR = exp(-abs(texture(TEXTURE_2, uv + UV_DELTA).r - BASE_DEPTH) * 100.0) * FACTOR; \
        LIGHT_VAR += texture(TEXTURE_1, uv + UV_DELTA) * F_VAR; \
        SUM_VAR += F_VAR; \
        NEXT_SAMPLE \
    }

#ifdef HIGH_QUALITY_SHADOWS
#define ADD_TO_BLEND(LIGHT_VAR, SUM_VAR, BASE_DEPTH, UV_DELTA, FACTOR) \
    ADD_TO_BLEND0(LIGHT_VAR, SUM_VAR, f0, BASE_DEPTH, UV_DELTA, FACTOR, { \
        ADD_TO_BLEND0(LIGHT_VAR, SUM_VAR, f1, BASE_DEPTH, UV_DELTA * 2, (FACTOR) * f0, {}) \
    })
#else
#define ADD_TO_BLEND(LIGHT_VAR, SUM_VAR, BASE_DEPTH, UV_DELTA, FACTOR) \
    ADD_TO_BLEND0(LIGHT_VAR, SUM_VAR, f0, BASE_DEPTH, UV_DELTA, FACTOR, {})
#endif

void main() {
    #ifdef SOFT_SHADOWS
        vec4 light = vec4(0.0);
        float s = 0.0;
        float base_depth = texture(TEXTURE_2, uv).r;
        ADD_TO_BLEND(light, s, base_depth, vec2(0, 0), MUL_0);
        ADD_TO_BLEND(light, s, base_depth, vec2(BLEND_RADIUS.x, 0), MUL_1);
        ADD_TO_BLEND(light, s, base_depth, vec2(-BLEND_RADIUS.x, 0), MUL_1);
        ADD_TO_BLEND(light, s, base_depth, vec2(0, BLEND_RADIUS.y), MUL_1);
        ADD_TO_BLEND(light, s, base_depth, vec2(0, -BLEND_RADIUS.y), MUL_1);
        ADD_TO_BLEND(light, s, base_depth, vec2(BLEND_RADIUS.x, BLEND_RADIUS.y), MUL_2);
        ADD_TO_BLEND(light, s, base_depth, vec2(-BLEND_RADIUS.x, BLEND_RADIUS.y), MUL_2);
        ADD_TO_BLEND(light, s, base_depth, vec2(BLEND_RADIUS.x, -BLEND_RADIUS.y), MUL_2);
        ADD_TO_BLEND(light, s, base_depth, vec2(-BLEND_RADIUS.x, -BLEND_RADIUS.y), MUL_2);
        light /= s;
    #else
        vec4 light = texture(TEXTURE_1, uv);
    #endif

    gl_FragColor = texture(TEXTURE_0, uv) * light;
}