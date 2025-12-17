#version 430

// model view projection of points
uniform mat4 mvp = mat4(1.0); 
// homog transform that brings points to world frame
uniform mat4 tx_world_object = mat4(1.0); 
uniform float point_size = 10.0f;
layout(location = 0) in vec3 point; 

out vec3 v_world_xyz;

void main()
{
    gl_PointSize = point_size;
    gl_Position = mvp * vec4(point, 1.0f);
    v_world_xyz = (tx_world_object * vec4(point, 1.0f)).xyz;
}
