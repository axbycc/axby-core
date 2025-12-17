#version 430

out vec4 output_color;

uniform sampler2D input_texture;
in vec2 texcoord;

void main()
{
    vec2 samplecoord = texcoord;
    output_color = texture(input_texture, samplecoord);
}
