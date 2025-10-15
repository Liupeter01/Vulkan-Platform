#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outUV;

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(push_constant) uniform PushConstant {
    mat4 render_matrix;
    VertexBuffer vertexBuffer;
} pc;

layout(set=0,binding=0) uniform SceneData {
  mat4 view;
 mat4 proj;
  vec4 ambientColor;
  vec4 sunlightDirection; // w for sun power
  vec4 sunlightColor;
} myScene;

void main() {

  // load vertex data from device address
    Vertex v = pc.vertexBuffer.vertices[gl_VertexIndex];

    // transform to clip space
    gl_Position = myScene.proj * myScene.view * pc.render_matrix * vec4(v.position, 1.0);

    // pass varying
    outColor = v.color.xyz;
    outUV = vec2(v.uv_x, v.uv_y);
}

