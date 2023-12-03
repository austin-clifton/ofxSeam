#version 150

uniform mat4 modelViewMatrix;
uniform vec3 cameraLeft;
uniform vec3 cameraUp;
uniform float minWidth = 0.2;

in vec4 position;
in vec4 color;
in vec3 velocity;

out vec4 geoColor;
out vec2 velocitySkew;
// out float distToCamera;

void main()
{
  vec4 mvPos = modelViewMatrix * position;
  gl_Position = mvPos;
  geoColor = color;

  float camUpDotVel = dot(cameraUp, velocity);
  float camLeftDotVel = dot(cameraLeft, velocity);
  camUpDotVel = pow(abs(camUpDotVel), 0.6) * sign(camUpDotVel);
  camLeftDotVel = pow(abs(camLeftDotVel), 0.6) * sign(camLeftDotVel);

  velocitySkew = vec2(
    abs(camLeftDotVel) > minWidth ? camLeftDotVel : minWidth,
    abs(camUpDotVel) > minWidth ? camUpDotVel : minWidth
  );
  
  // distToCamera = -mvPos.z / mvPos.w;
}