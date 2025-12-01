#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

//
//output to frag
layout(location = 0) out vec2 fragOffset;

const vec2 OFFSETS[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, -1.0),
    vec2(1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, 1.0)
  );

layout(push_constant) uniform PushConstant {
  vec4 position;
  vec4 color;
  float radius;
} push;

const int MAX_LIGHTS = 10;

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
  fragOffset = OFFSETS[gl_VertexIndex];

  //
  vec3 cameraRight = {
      ubo.V[0][0],
      ubo.V[1][0],
      ubo.V[2][0]
    };
  vec3 cameraUp = {
      ubo.V[0][1],
      ubo.V[1][1],
      ubo.V[2][1]
    };

  vec3 worldPosition = push.position.xyz
      + fragOffset.x * push.radius * cameraRight
      + fragOffset.y * push.radius * cameraUp;

  gl_Position = ubo.P * ubo.V * vec4(worldPosition, 1.0);
}
