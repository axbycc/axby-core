#version 430

flat in uint v_point_id;
flat in uint v_group_id;

// render into GL_RG32UI frame buffer
layout(location = 0) out uvec2 out_id;

void main() {
    out_id = uvec2(v_group_id, v_point_id);
}
