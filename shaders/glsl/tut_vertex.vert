#version 450

//input
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec2 uv;

//output to frag
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPositionWorld;
layout(location = 2) out vec3 fragNormalWorld;

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

void main() {
    vec4 worldPosition = push.Model * vec4(position, 1.0);

    fragNormalWorld = normalize(mat3(push.NormalMatrix) * normal);
    fragPositionWorld = worldPosition.xyz;
    fragColor = color;

    gl_Position = ubo.P * ubo.V * worldPosition;
}
