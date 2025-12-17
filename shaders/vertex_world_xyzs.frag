#version 430

in vec3 v_world_xyz;

layout (location = 0) out vec3 out_world_xyz;

void main() {
    out_world_xyz = v_world_xyz;
}
    
