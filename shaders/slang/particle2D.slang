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
    // float3 pad0;
};

struct SlangArgs {
    float4 position : SV_Position;
    float4 color;
}

float2 reflectBoundaryXY2D(float2 pos, inout float2 vel, float boundary)
{
    if (pos.x <= -boundary) {
        pos.x = -boundary;
        vel.x = abs(vel.x);
    }
    else if (pos.x >= boundary) {
        pos.x = boundary;
        vel.x = -abs(vel.x);
    }

    if (pos.y <= -boundary) {
        pos.y = -boundary;
        vel.y = abs(vel.y);
    }
    else if (pos.y >= boundary) {
        pos.y = boundary;
        vel.y = -abs(vel.y);
    }

    return pos;
}

// <binding, set>
[[vk::binding(0, 0)]]
StructuredBuffer<ParticleSSBO> particlesIn;

[[vk::binding(1, 0)]]
RWStructuredBuffer<ParticleSSBO> particlesOut;

[[vk::binding(2, 0)]]
RWTexture2D<float4> image;

[[vk::push_constant]]
ParticlePushConstant pc;

[shader("vertex")]
SlangArgs vertMain(uint VertexID: SV_VertexID) {
    Particle p = particlesIn[VertexID].particles;
    SlangArgs o;
    o.position = p.position;
    o.color = p.color;
    return o;
}

[shader("fragment")]
float4 fragMain(in SlangArgs args) : SV_Target {
    return args.color;
}

[shader("compute")]
[numthreads(256,1,1)]
void compMain(uint3 threadId : SV_DispatchThreadID)
{
    uint index = threadId.x;
    uint width, height;
    image.GetDimensions(width, height);
    float2 imageSizeF = float2(width, height);
    //float ax = (float)height / (float)width; // = 1/aspect

    float2 pos = particlesIn[index].particles.position.xy;
    float2 vel = particlesIn[index].particles.velocity.xy;

    float xyboundary = 0.97;
    pos.xy = reflectBoundaryXY2D(pos.xy, vel.xy, xyboundary);

    pos += vel * pc.deltaTime * pc.speedup.xy;

    particlesOut[index].particles.position.xy = pos;
    particlesOut[index].particles.velocity.xy = vel;

    float4 posColor = particlesOut[index].particles.color;
    float2 posNDC = particlesOut[index].particles.position.xy;

    uint2 pixel = uint2((pos + 1.0) * 0.5 * imageSizeF);
    pixel = clamp(pixel, uint2(0, 0), uint2(width - 1, height - 1));

    image[pixel] = particlesIn[index].particles.color;
}