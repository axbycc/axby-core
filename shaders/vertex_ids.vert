#version 430

layout(location = 0) in vec3 point; // x,y,z

uniform mat4 mvp = mat4(1.0); // model view projection matrix
uniform uint group_id = 0;
uniform float point_size = 10.0f;

flat out uint v_point_id;
flat out uint v_group_id;

void main() {
    gl_PointSize = point_size;
    gl_Position = mvp * vec4(point, 1.0f);
    v_point_id = gl_VertexID;
    v_group_id = group_id;
}
