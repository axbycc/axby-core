#version 430

layout(location = 0) in vec3 point;   
layout(location = 1) in vec3 color;   
layout(location = 2) in vec3 normal; 

uniform mat4 tx_camera_object; // view*model matrix
uniform mat4 ndc_image_camera; // projection matrix

out vec3 frag_position; // Position in camera space
out vec3 frag_normal; // Normal in camera space
out vec3 vertex_color;

void main() {
    // Calculate transformed position and normal
    vec4 point_camera = tx_camera_object * vec4(point, 1.0);
    frag_position = point_camera.xyz;

    mat3 rot_camera_object = mat3(tx_camera_object);
    frag_normal = rot_camera_object * normal;

    vertex_color = color;
    gl_Position = ndc_image_camera * point_camera;
}
