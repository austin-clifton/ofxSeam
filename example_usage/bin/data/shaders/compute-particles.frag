#version 150

in vec2 vertexUv;
in vec4 vertexColor;

out vec4 fragColor;

void main (void)
{
    float dist_from_center = distance(vertexUv, vec2(0.5f));

    fragColor = vec4(vec3(1.0) * (1.0 - smoothstep(0.0, 0.5, dist_from_center)), 1.0); // vertexColor;
}