struct Particle {
    float4 position;
    float4 velocity;
    float4 color;
};

struct ParticleSSBO {
    Particle particles;
};

struct ParticlePushConstant {
    float4 speedup;
    float deltaTime;
};

struct SlangArgs {
    float4 position : SV_Position;
    float PSIZE : SV_PointSize;
    float4 color;
}

// layout(set = 0, binding = 0)
[[vk::binding(0, 0)]]
StructuredBuffer<ParticleSSBO> particlesIn;

// layout(set = 0, binding = 1)
[[vk::binding(1, 0)]]
RWStructuredBuffer<ParticleSSBO> particlesOut;

[[vk::push_constant]]
ParticlePushConstant pc;

[shader("vertex")]
SlangArgs vertMain(uint vertexId: SV_VertexID){
    SlangArgs o;
    o.position = particlesIn[vertexId].particles.position;
    o.color = particlesIn[vertexId].particles.color;
    o.PSIZE = 10;
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
    uint width, height;

    float3 pos = particlesIn[index].particles.position.xyz;
    float3 vel = particlesIn[index].particles.velocity.xyz;

    const float boundary = 0.97;

    if (abs(pos.x) > boundary) {
        pos.x = pos.x < 0 ? -boundary : boundary;
        vel.x = vel.x < 0 ? abs(vel.x) : -abs(vel.x);
    }

    if (abs(pos.y) > boundary) {
        pos.y = pos.y < 0 ? -boundary : boundary;
        vel.y = vel.y < 0 ? abs(vel.y) : -abs(vel.y);
    }

    if (abs(pos.z) > boundary) {
        pos.z = pos.z < 0 ? -boundary : boundary;
        vel.z = vel.z < 0 ? abs(vel.z) : -abs(vel.z);
    }

    pos += vel * pc.deltaTime * pc.speedup.xyz;

    particlesOut[index].particles.position.xyz = pos;
    particlesOut[index].particles.velocity.xyz = vel;
}