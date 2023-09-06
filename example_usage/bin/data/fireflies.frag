#version 150

uniform float bubbleRadius = 0.45;
uniform float bubbleLineWidth = 0.05;
uniform float eyeRadius = 0.1;
uniform float eyeLineWidth = 0.05;

in vec2 vertexUv;
in vec4 vertexColor;

out vec4 color;

void main (void)
{
    float distFromCenter = distance(vertexUv, vec2(0.5f));

    float bubbleLine = smoothstep(bubbleRadius - bubbleLineWidth, bubbleRadius, distFromCenter)
        - smoothstep(bubbleRadius, bubbleRadius + bubbleLineWidth, distFromCenter);

    float fill = smoothstep(0, bubbleRadius, distFromCenter) * ( 1.0 - step(bubbleRadius, distFromCenter)) * 0.5;

    vec2 eyeCenter = vec2(0.5, 0.5 + bubbleRadius - bubbleLineWidth - eyeRadius - (eyeLineWidth / 2));
    float distFromEye = distance(vertexUv, eyeCenter);

    float eyeLine = smoothstep(eyeRadius - eyeLineWidth, eyeRadius, distFromEye)
        - smoothstep(eyeRadius, eyeRadius + eyeLineWidth, distFromEye);

    color = bubbleLine * vertexColor + fill * vertexColor + eyeLine * vertexColor;
}