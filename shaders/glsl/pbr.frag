//glsl version 4.5
#version 450
#extension GL_KHR_vulkan_glsl : enable
#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

//shader input
layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

//output write
layout(location = 0) out vec4 outFragColor;

void main()
{
  float lightValue = clamp(dot(inNormal, myScene.sunlightDirection.xyz), 0.1f, 1.f);

  vec3 color = inColor * texture(colorTex, inUV).xyz;
  vec3 ambient = color * myScene.ambientColor.xyz;

  outFragColor = vec4(color * lightValue * myScene.sunlightColor.w + ambient, 1.0f);
}
