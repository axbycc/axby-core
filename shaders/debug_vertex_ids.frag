#version 430

in vec2 texcoord;

layout(binding = 0) uniform usampler2D vertex_ids_texture;
layout(location = 0) out vec3 out_color;

uint wang_hash(uint x) {
    x = (x ^ 61u) ^ (x >> 16u);
    x *= 9u;
    x = x ^ (x >> 4u);
    x *= 0x27d4eb2du;
    x = x ^ (x >> 15u);
    return x;
}

vec3 id_to_color(uint id) {
    uint h = wang_hash(id);
    vec3 c = vec3(
        float((h >>  0) & 255u) / 255.0,
        float((h >>  8) & 255u) / 255.0,
        float((h >> 16) & 255u) / 255.0
        );
    // Bias a bit toward brighter colors
    c = mix(c, vec3(1.0), 0.25);
    return c;
}


void main() {
    uvec2 ids = texture(vertex_ids_texture, texcoord).xy;
    uint group_id = ids.x;
    uint point_id = ids.y;
    vec3 group_color = id_to_color(group_id);
    vec3 point_color = id_to_color(point_id);
    vec3 rgb = mix(group_color, point_color, 0.1);
    out_color = rgb;
}
