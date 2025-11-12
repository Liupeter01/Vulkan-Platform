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