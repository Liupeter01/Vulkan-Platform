#version 450

////input
//layout(location = 0) in vec3 position;
//layout(location = 1) in vec3 color;
//layout(location = 2) in vec3 normal;
//layout(location = 3) in vec2 uv;

//input
layout(location = 0) in vec2 fragOffset;

//output
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstant {
  vec4 position;
  vec4 color;
  float radius;
} push;

const int MAX_LIGHTS = 10;
const float pi = 3.141592653;

struct PointLight {
  vec4 position;
  vec4 color;
};

layout(set = 0, binding = 0) uniform GlobalUBO {
  mat4 P;
  mat4 V;
  mat4 inv_V; //for camera model matrix
  vec4 ambientColor;
  PointLight pointLights[MAX_LIGHTS];
  int numLights;
} ubo;

void main() {
  float _distance = sqrt(dot(fragOffset, fragOffset));
  if (_distance >= 1) {
    discard; //only in fragment
  }

  outColor = vec4(push.color.xyz, 0.5 * cos(_distance * pi) + 1.f);
}
