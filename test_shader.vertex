#version 330 core

layout (location = 0) in vec3 POSITION;
layout (location = 1) in vec4 COLOR;
layout (location = 2) in vec2 UV;

varying vec2 uv;

void main() {
    uv = UV;
    gl_Position = vec4(POSITION, 1.0);
}