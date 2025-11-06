//glsl version 4.5
#version 450
#extension GL_KHR_vulkan_glsl : enable

//shader input
layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inUV;

//output write
layout(location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform SceneData {
  mat4 view;
  mat4 proj;
  vec4 ambientColor;
  vec4 sunlightDirection; // w for sun power
  vec4 sunlightColor;
} myScene;

layout(set = 1, binding = 0) uniform sampler2D displayTexture;

void main()
{
  outFragColor = texture(displayTexture, inUV);
}
