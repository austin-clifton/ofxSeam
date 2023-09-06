#version 150

// particle billboarding with geometry shader, repurposed from this post:
// https://www.geeks3d.com/20140815/particle-billboarding-with-the-geometry-shader-glsl/

const float PI = 3.1415926535; 

layout (points) in;
layout (triangle_strip) out;
layout (max_vertices = 4) out;

uniform mat4 projectionMatrix;
   
uniform float particleSize = 4.0f;

uniform float pulseDistance = -10.0;
uniform float pulseRange = 10.0;
uniform float pulseSizeMultiplier = 4.0;

in vec4 geoColor[];
in vec2 velocitySkew[];

out vec2 vertexUv;
out vec4 vertexColor;

mat2 rotate2d(float angle) {
  float sa = sin(angle);
  float ca = cos(angle);
  return mat2(ca, -sa, sa, ca);
}
   
void main (void)
{
  vec4 P = gl_in[0].gl_Position;

  float angle = PI/8;
  mat2 rotation = rotate2d(angle + pow(1.0 - velocitySkew[0].x, 5));

  vec4 pointPos = projectionMatrix * vec4(P.xy, P.zw);
  float distToCamera = (2.0 * -P.z - gl_DepthRange.near) 
    / (gl_DepthRange.far - gl_DepthRange.near);

  float pulseMultiplier = 1.0
    + (smoothstep(pulseDistance - pulseRange, pulseDistance, distToCamera)
    - smoothstep(pulseDistance, pulseDistance + pulseRange, distToCamera)) * pulseSizeMultiplier;

  // a: left-bottom 
  vec2 va = P.xy + vec2(-0.5, -0.5) * velocitySkew[0] * rotation * particleSize * pulseMultiplier;
  gl_Position = projectionMatrix * vec4(va, P.zw);
  vertexUv = vec2(0.0, 0.0);
  vertexColor = geoColor[0];
  EmitVertex();  
  
  // b: left-top
  vec2 vb = P.xy + vec2(-0.5, 0.5) * velocitySkew[0] * rotation * particleSize * pulseMultiplier;
  gl_Position = projectionMatrix * vec4(vb, P.zw);
  vertexUv = vec2(0.0, 1.0);
  vertexColor = geoColor[0];
  EmitVertex();  
  
  // d: right-bottom
  vec2 vd = P.xy + vec2(0.5, -0.5) * velocitySkew[0] * rotation * particleSize * pulseMultiplier;
  gl_Position = projectionMatrix * vec4(vd, P.zw);
  vertexUv = vec2(1.0, 0.0);
  vertexColor = geoColor[0];
  EmitVertex();  

  // c: right-top
  vec2 vc = P.xy + vec2(0.5, 0.5) * velocitySkew[0] * rotation * particleSize * pulseMultiplier;
  gl_Position = projectionMatrix * vec4(vc, P.zw);
  vertexUv = vec2(1.0, 1.0);
  vertexColor = geoColor[0];
  EmitVertex();  

  EndPrimitive();  
}   