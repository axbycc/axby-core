#version 430

out vec3 output_color;

uniform sampler2D input_texture;
uniform sampler2D cmap_texture;

uniform float cmap_max = 1.0f;
uniform float cmap_min = 0.0f;
uniform float cmap_scale = 1.0f;
uniform bool cmap_invert = false;

in vec2 texcoord;

void main()
{
    float data_value = texture(input_texture, texcoord).r * cmap_scale;
    if ((data_value <= cmap_min) || (data_value >= cmap_max)) {
        output_color = vec3(0.0f, 0.0f, 0.0f);
        return;
    }

    data_value = clamp(data_value, cmap_min, cmap_max)/(cmap_max - cmap_min);
    if (cmap_invert) {
        data_value = 1.0f - data_value;
    }
    output_color = texture(cmap_texture, vec2(data_value, 0.5)).rgb;
}
