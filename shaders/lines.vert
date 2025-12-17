#version 430

uniform mat4 mvp1 = mat4(1.0); // model view projection of points 1
uniform mat4 mvp2 = mat4(1.0); // model view projection of points 2

layout(location = 0) in vec3 point1; 
layout(location = 1) in vec3 point2;
layout(location = 2) in vec3 color;
layout(location = 3) in float alpha;
 
out vec4 v_color;

void main()
{
    v_color = vec4(color, alpha);

    vec3 pos;
    mat4 mvp;    
    if (gl_VertexID == 0) {
        pos = point1;
        mvp = mvp1;
    } else {
        pos = point2;
        mvp = mvp2;
    }

    gl_Position = mvp * vec4(pos, 1.0);
}
