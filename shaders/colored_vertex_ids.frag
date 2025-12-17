#version 430

flat in uint v_point_id;
flat in uint v_group_id;

layout(location = 0) out vec4 out_color;

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
    return vec3(
        float((h >>  0) & 255u) / 255.0,
        float((h >>  8) & 255u) / 255.0,
        float((h >> 16) & 255u) / 255.0
    );
}

void main() {
    vec3 group_color = id_to_color(v_group_id);
    vec3 point_color = id_to_color(v_point_id);
    vec3 final_color = mix(group_color, point_color, 0.3f);
    out_color = vec4(final_color, 1.0f);
}
