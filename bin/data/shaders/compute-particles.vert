#version 150

uniform mat4 modelViewMatrix;

in vec4 position;

void main()
{
  gl_Position = modelViewMatrix * position;
}