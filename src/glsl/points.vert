#version 330 core

in vec3 a_pos;
in vec3 a_color;

out vec3 v_color;

uniform mat4 MVP;

void main() {
    v_color = a_color;
    gl_Position = MVP * vec4(a_pos, 1.0);
}
