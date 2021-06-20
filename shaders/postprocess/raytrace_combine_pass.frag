#version 330 core

uniform sampler2D IN_TEXTURE_COLOR;

varying vec2 uv;

void main() {
    vec4 result = vec4(1.0);

    result *= texture(IN_TEXTURE_COLOR, uv);

    gl_FragColor = result;
}