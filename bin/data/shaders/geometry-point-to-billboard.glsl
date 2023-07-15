#version 150

// particle billboarding with geometry shader, repurposed from this post:
// https://www.geeks3d.com/20140815/particle-billboarding-with-the-geometry-shader-glsl/

layout (points) in;
layout (triangle_strip) out;
layout (max_vertices = 4) out;

uniform mat4 projectionMatrix;
   
uniform float particleSize = 1.0f;

in float vSize[];

in Vertex
{
  vec4 color;
  vec4 vel;
  float theta;
  float mass;
  float sizeModifier;
  float unused;
} v[];

out vec2 vertexUv;
out vec4 vertexColor;
   
void main (void)
{
  vec4 P = gl_in[0].gl_Position;

  // a: left-bottom 
  vec2 va = P.xy + vec2(-0.5, -0.5) * particleSize * vSize[0];
  gl_Position = projectionMatrix * vec4(va, P.zw);
  vertexUv = vec2(0.0, 0.0);
  vertexColor = v[0].color;
  EmitVertex();  
  
  // b: left-top
  vec2 vb = P.xy + vec2(-0.5, 0.5) * particleSize * vSize[0];
  gl_Position = projectionMatrix * vec4(vb, P.zw);
  vertexUv = vec2(0.0, 1.0);
  vertexColor = v[0].color;
  EmitVertex();  
  
  // d: right-bottom
  vec2 vd = P.xy + vec2(0.5, -0.5) * particleSize * vSize[0];
  gl_Position = projectionMatrix * vec4(vd, P.zw);
  vertexUv = vec2(1.0, 0.0);
  vertexColor = v[0].color;
  EmitVertex();  

  // c: right-top
  vec2 vc = P.xy + vec2(0.5, 0.5) * particleSize * vSize[0];
  gl_Position = projectionMatrix * vec4(vc, P.zw);
  vertexUv = vec2(1.0, 1.0);
  vertexColor = v[0].color;
  EmitVertex();  

  EndPrimitive();  
}   