#version 430

// 2 triangles (6 vertices) creating a quad of dimension 1x1
const vec2 positions[6] = vec2[6](
    // triangle 1
    vec2(-0.5, -0.5), 
    vec2( 0.5, -0.5),
    vec2(-0.5,  0.5),
    // triangle 2
    vec2(-0.5,  0.5), 
    vec2( 0.5, -0.5),
    vec2( 0.5,  0.5)
);

uniform mat4 mvp = mat4(1.0); // model view projection

out vec2 texcoord;

void main() {
    vec2 quadcoord = positions[gl_VertexID];
    texcoord = (quadcoord + 0.5); // (-0.5 to 0.5) => (0 to 1)

    vec4 quadcoord_homog = vec4(quadcoord.xy, 0, 1);
    gl_Position = mvp * quadcoord_homog;
}
