#version 430 core

layout(location = 0) in vec3 point;

uniform mat4 mvp = mat4(1.0);
uniform uint group_id = 0;

flat out uint v_group_id;
flat out uint v_point_id;

void main() {
    gl_PointSize = 10.0f;
    gl_Position = mvp * vec4(point, 1.0f);
    v_group_id = group_id;
    v_point_id = gl_VertexID;
}
