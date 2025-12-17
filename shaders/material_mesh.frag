#version 430

in vec3 frag_position;
in vec3 frag_normal;
in vec3 vertex_color;

struct Material {
    vec3 diffuse;
    vec3 specular;
    vec3 ambient;
    vec3 emissive;
    float opacity;
    float specular_exponent;
};

uniform Material material;


uniform vec3 light_direction = normalize(vec3(-0.5, -1.0, -0.5));
uniform vec3 light_direction_floor = normalize(vec3(-0, -0, -1));

out vec4 output_color;

void main() {
    vec3 normal = normalize(frag_normal);
    vec3 diffuse_color = max(dot(normal, light_direction), 0.0) * vertex_color * material.diffuse;
    vec3 view_direction = normalize(-frag_position);
    vec3 reflect_direction = reflect(-light_direction, normal);
    float view_reflect_alignment = max(dot(view_direction, reflect_direction), 0.0);
    float specular_exponent = max(material.specular_exponent, 1.0f);
    vec3 specular_color = pow(view_reflect_alignment, specular_exponent) * material.specular;

    float ambient_strength = 0.05;
    vec3 ambient_color = ambient_strength * material.ambient;

    // add another light source to model reflected light from the floor
    vec3 floor_base_color = max(dot(normal, light_direction_floor), 0.0) * vertex_color * material.diffuse;
    float floor_light_strength = 0.3;
    vec3 floor_color= floor_base_color * floor_light_strength;

    vec3 result = (ambient_color + diffuse_color + specular_color + floor_color);
    output_color = vec4(result, material.opacity);
}
