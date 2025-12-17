#version 430 core

layout(location = 0) in vec3 point;
layout(location = 1) in vec3 color;
layout(location = 2) in float alpha;

uniform vec3 tint_color = vec3(0,0,0);
uniform float tint_amount = 0.0f;
uniform mat4 mvp = mat4(1.0);
uniform float point_size = 10.0f;
out vec4 vertex_color;

void main() {
    const vec4 ndc_position = mvp * vec4(point.x, point.y, point.z, 1.0);
    vertex_color.rgb = (1-tint_amount)*color + tint_amount*tint_color;
    vertex_color.a = alpha;

    gl_PointSize = point_size;  // make points large enough to see    
    gl_Position = ndc_position;
}
