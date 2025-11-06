#version 450

//input to frag
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPositionWorld;
layout(location = 2) in vec3 fragNormalWorld;

//output
layout(location = 0) out vec4 outColor;

//constant
layout(push_constant) uniform Push {
  mat4 Model;
  mat4 NormalMatrix;
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

vec3 diffuse(const vec3 faceNormal) {
  vec3 blend = vec3(0.f);
  for (int i = 0; i < ubo.numLights; ++i) {
    PointLight light = ubo.pointLights[i];
    vec3 lightDir = fragPositionWorld - light.position.xyz;
    float _distance = 1.f / dot(lightDir, lightDir);
    float cosAngle = clamp(dot(faceNormal, -normalize(lightDir)), 0.f, 1.f);
    blend += cosAngle * _distance * light.color.xyz * light.color.w;
  }
  return blend;
}

vec3 specular(const vec3 faceNormal, const vec3 cameraPositionWorld) {
  vec3 blend = vec3(0.f);

  const vec3 shadeToEye = normalize(cameraPositionWorld - fragPositionWorld);

  for (int i = 0; i < ubo.numLights; ++i) {
    PointLight light = ubo.pointLights[i];
    vec3 lightDir = fragPositionWorld - light.position.xyz;
    float _distance = 1.f / dot(lightDir, lightDir);

    vec3 h = normalize(-normalize(lightDir) + shadeToEye);

    float halfCosAngle = clamp(dot(faceNormal, h), 0.f, 1.f);
    blend += light.color.xyz * light.color.w * pow(halfCosAngle, 64.f) * _distance;
  }
  return blend;
}

void main() {
  const vec3 ambientColor = ubo.ambientColor.xyz * ubo.ambientColor.w;
  const vec3 faceNormal = normalize(fragNormalWorld);
  const vec3 cameraPositionWorld = ubo.inv_V[3].xyz;

  const vec3 diffuse_color = diffuse(faceNormal);
  const vec3 specular_color = specular(faceNormal, cameraPositionWorld);

  outColor = vec4((diffuse_color + specular_color + ambientColor) * fragColor, 1.0);
}
