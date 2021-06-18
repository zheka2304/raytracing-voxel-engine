#version 330

layout (location = 0) in vec3 POSITION;
layout (location = 1) in vec4 COLOR;

varying vec4 color;

void main() {
    color = COLOR;
    gl_Position = vec4(POSITION, 1.0);
}