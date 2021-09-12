
#ifndef VOXEL_ENGINE_UTIL_INC
#define VOXEL_ENGINE_UTIL_INC



// Color pairs allow you to pack two vec3 colors into one 32 bit uvec3 or vec3 and then unpack them.
struct ColorPair {
    vec3 color1;
    vec3 color2;
};

uvec3 packColorPair(vec3 color1, vec3 color2) {
    return uvec3(
        packHalf2x16(vec2(color1.r, color2.r)),
        packHalf2x16(vec2(color1.g, color2.g)),
        packHalf2x16(vec2(color1.b, color2.b))
    );
}

vec3 packColorPairF(vec3 color1, vec3 color2) {
    return uintBitsToFloat(packColorPair(color1, color2));
}

ColorPair unpackColorPair(uvec3 pair) {
    vec2 r = unpackHalf2x16(pair.r);
    vec2 g = unpackHalf2x16(pair.g);
    vec2 b = unpackHalf2x16(pair.b);
    return ColorPair(vec3(r.x, g.x, b.x), vec3(r.y, g.y, b.y));
}

ColorPair unpackColorPairF(vec3 pair) {
    return unpackColorPair(floatBitsToUint(pair));
}

float fast_random(float x) {
    return fract(sin(x * 12.9898) * 43758.5453);
}

float fast_random(vec2 x) {
    return fract(sin(dot(x, vec2(12.9898, 78.233) * 1213.232)) * 43758.5453);
}

float fast_random(vec3 x) {
    return fract(sin(dot(x, vec3(12.9898, 78.233, 64.283) * 1213.232)) * 43758.5453);
}

float hash(vec2 p) { return fract(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x)))); }

float fast_random(vec4 x) {
    return hash(vec2(dot(x.xy, x.zw), dot(x.xz, x.yw) + 123.0));
}


#endif  // VOXEL_ENGINE_UTIL_INC