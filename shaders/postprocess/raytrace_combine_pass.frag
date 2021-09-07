#version 330 core

uniform sampler2D IN_TEXTURE_COLOR;
uniform sampler2D IN_TEXTURE_LIGHT;
uniform sampler2D IN_TEXTURE_DEPTH;

varying vec2 uv;

void main() {
    vec4 result = vec4(1.0);

    result *= texture(IN_TEXTURE_COLOR, uv);

    gl_FragColor = result;
}