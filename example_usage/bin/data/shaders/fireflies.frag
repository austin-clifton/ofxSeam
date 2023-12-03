#version 150

uniform float particleRadius = 0.3;
uniform float glowRadius = 0.48;
uniform vec2 particleLuminosity = vec2(0.7, 0.3);
uniform vec2 glowLuminosity = vec2(0.3, 0.0);

in vec2 vertexUv;
in vec4 vertexColor;

out vec4 color;

float RangeFrom01(float v, vec2 range) {
    return range.x + (range.y - range.x) * v;
}

void main (void)
{
    // vec2 vertexUv = gl_FragCoord.xy / vec2(1080.0, 1080.0);
    
    float distFromCenter = distance(vertexUv, vec2(0.5f));

    float inParticle = step(distFromCenter, particleRadius);
    float inGlow = (1.0 - inParticle) * step(distFromCenter, glowRadius);

    float nParticleDist = distFromCenter / particleRadius;
    float nGlowDist = (distFromCenter - particleRadius) / (glowRadius - particleRadius);

    float particle = inParticle * RangeFrom01(nParticleDist, particleLuminosity);
    float glow = inGlow * RangeFrom01(nGlowDist, glowLuminosity);

    color = (particle + glow) * vertexColor;
}