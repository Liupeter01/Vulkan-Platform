struct SSBO {
    float4 items;
};

struct ParticlePushConstant {
    float4 speedup;
    float deltaTime;
};

// layout(set = 0, binding = 0)
[[vk::binding(0, 0)]]
StructuredBuffer<SSBO> particlesPositionIn;

// layout(set = 0, binding = 1)
[[vk::binding(1, 0)]]
RWStructuredBuffer<SSBO> particlesPositionOut;

// layout(set = 0, binding = 2)
[[vk::binding(2, 0)]]
RWStructuredBuffer<SSBO> particlesVelocityIn;

// layout(set = 0, binding = 3)
[[vk::binding(3, 0)]]
RWStructuredBuffer<SSBO> particlesVelocityOut;

// layout(set = 0, binding = 4)
[[vk::binding(4, 0)]]
RWStructuredBuffer<SSBO> particlesColorIn;

// layout(set = 0, binding = 5)
[[vk::binding(5, 0)]]
RWStructuredBuffer<SSBO> particlesColorOut;

[[vk::push_constant]]
ParticlePushConstant pc;

[shader("vertex")]
void vertMain(void) {
}

[shader("fragment")]
void fragMain(void) {
}

[shader("compute")]
[numthreads(256, 1, 1)]
void compMain(uint3 threadId: SV_DispatchThreadID) {
    uint index = threadId.x;
    uint width, height;

    float3 pos = particlesPositionIn[index].items.xyz;
    float3 vel = particlesVelocityIn[index].items.xyz;
    //float4 color = particlesColorIn[index].items;

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

    particlesPositionOut[index].items.xyz = pos;
    particlesVelocityOut[index].items.xyz = vel;
}