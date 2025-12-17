#version 430 core

out vec4 output_color;
in vec4 vertex_color;

void main() {
    output_color.rgb = vertex_color.rgb;
    output_color.a = vertex_color.a;
}
