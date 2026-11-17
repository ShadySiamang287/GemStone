#include "common.hlsli"

cbuffer FrameData : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4x4 invViewMatrix;
    float4x4 invProjectionMatrix;
    int lightCount;
    float3 _padding;
}

RaytracingAccelerationStructure accelStructure : register(t0);
StructuredBuffer<PointLight> pointLights : register(t1);
Texture2D<float> depthTexture : register(t2);

RWTexture2D<float4> outputTexture : register(u0);


float3 ReconstructWorldPosition(float2 texCoord, float depth) {
    float2 ndc = texCoord * 2.0 - 1.0;
    ndc.y = 1.0f - texCoord.y * 2.0f;
    float4 clipSpacePosition = float4(ndc, depth, 1.0);

    float4 viewPosition = mul(invProjectionMatrix, clipSpacePosition);
    viewPosition /= viewPosition.w;

    float4 worldPosition = mul(invViewMatrix, viewPosition);
    return worldPosition.xyz;
}

struct ShadowPayload {
    bool hit;
};

[shader("raygeneration")]
void shadowRayGen() {

    uint2 coords = DispatchRaysIndex().xy;
    uint2 dims = DispatchRaysDimensions().xy;

    float depthValue = depthTexture[coords];

    if (depthValue == 0.f) {
        outputTexture[coords] = float4(1, 1, 1, 1);
        return;
    }

    float2 uv = (float2(coords) + 0.5) / float2(dims);
    float3 worldPos = ReconstructWorldPosition(uv, depthValue);

    float4 vis = float4(1, 1, 1, 1); // 1 = fully lit
    int count = min(lightCount, 4);

    for (int i = 0; i < 4; ++i) {
        float3 toLight = pointLights[i].positionRange.xyz - worldPos;
        float dist = length(toLight);

        if (dist > pointLights[i].positionRange.w)
            continue;

        RayDesc ray;
        ray.Origin = worldPos;
        ray.Direction = toLight / dist;
        ray.TMin = max(1.f, length(worldPos)) * 1e-3f;
        ray.TMax = dist;

        ShadowPayload payload;
        payload.hit = true;

        TraceRay(accelStructure,
                 RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_CULL_NON_OPAQUE,
                 0xFF,
                 0,
                 0,
                 0,
                 ray,
                 payload
        );

        vis[i] = payload.hit ? 0.0f : 1.0f;
    }

    outputTexture[coords] = vis;
}

[shader("miss")]
void miss(inout ShadowPayload payload) {
    payload.hit = false;
}