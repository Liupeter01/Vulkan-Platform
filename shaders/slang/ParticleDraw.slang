struct Particle {
    float4 position;
    float4 velocity;
    float4 color;
};

struct ParticleSSBO {
    Particle particles;
};

struct SlangArgs {
    float4 position : SV_Position;
    float PSIZE : SV_PointSize;
    float4 color;
}

struct SceneData {
    float4x4 view;
    float4x4 proj;
    float4 ambientColor;
    float4 sunlightDirection; // w for sun power
    float4 sunlightColor;
};

// layout(set = 0, binding = 0)
[[vk::binding(0, 0)]]
ConstantBuffer<SceneData> myScene;

// layout(set = 1, binding = 0)
[[vk::binding(0, 1)]]
StructuredBuffer<ParticleSSBO> particlesIn;

// layout(set = 1, binding = 1)
[[vk::binding(1, 1)]]
RWStructuredBuffer<ParticleSSBO> particlesOut;

[shader("vertex")]
SlangArgs vertMain(uint vertexId: SV_VertexID){
    SlangArgs o;

    o.position =  mul(myScene.proj, mul(myScene.view, float4(particlesIn[vertexId].particles.position.xyz, 1)));
    o.color = particlesIn[vertexId].particles.color;
    o.PSIZE = 2;
    return o;
}

[shader("fragment")]
float4 fragMain(in SlangArgs args) : SV_Target {
        return args.color;
}

[shader("compute")]
[numthreads(256, 1, 1)]
void compMain(uint3 threadId: SV_DispatchThreadID) {

    uint index = threadId.x;
}