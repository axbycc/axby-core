#version 430

const vec2 positions[6] = vec2[6](
    // triangle 1
    vec2(-0.5f, -0.5f), 
    vec2( 0.5f, -0.5f),
    vec2(-0.5f,  0.5f),
    // triangle 2
    vec2(-0.5f,  0.5f), 
    vec2( 0.5f, -0.5f),
    vec2( 0.5f,  0.5f)
);

out vec2 texcoord;

void main() {
    gl_PointSize = 10.0;  // make points large enough to see
    vec2 quadcoord = positions[gl_VertexID];
    gl_Position = vec4(quadcoord*2, 0.0f, 1.0f); // full screen
    texcoord = (quadcoord + 0.5); // (-0.5 to 0.5) => (0 to 1)
}
