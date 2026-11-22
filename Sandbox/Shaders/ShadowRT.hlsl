#include "common.hlsli"

cbuffer FrameData : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4x4 invViewMatrix;
    float4x4 invProjectionMatrix;
    float3 ambientLight;
    float ambientStrength;
    int lightCount;
    float3 padding;
}

RaytracingAccelerationStructure accelStructure : register(t0);
StructuredBuffer<PointLight> pointLights : register(t1);
Texture2D<float> depthTexture : register(t2);
Texture2D<float4> normalTexture : register(t3);

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

float Noise(float2 p)
{
    return frac(52.9829189 * frac(dot(p, float2(0.06711056, 0.00583715))));
}

float Occlusion(float3 origin, float3 dir, float tMax) {
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = dir;
    ray.TMin = 0.0f;
    ray.TMax = tMax;

    ShadowPayload payload;
    payload.hit = true;

    TraceRay(accelStructure,
             RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
                 RAY_FLAG_SKIP_CLOSEST_HIT_SHADER |
                 RAY_FLAG_CULL_NON_OPAQUE,
             0xFF, 0, 0, 0, ray, payload);

    return payload.hit ? 0.0f : 1.0f; // 1 = lit
}

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
    float3 normalSample = normalTexture[coords].rgb;
    float3 normal = normalize(normalSample.xyz * 2.0f - 1.0f);
    
    float3 camPos = float3(invViewMatrix[0][3], invViewMatrix[1][3], invViewMatrix[2][3]);

    float camDist = length(worldPos - camPos);
    float bias = max(0.13, camDist * 1e-3);

    float4 vis = float4(1, 1, 1, 1); // 1 = fully lit
    int count = min(lightCount, 4);
    float3 origin = worldPos + normal * bias;

    float angle0 = Noise(float2(coords)) * 6.2831853f;

    for (int i = 0; i < count; ++i) {
        float3 lightPos = pointLights[i].positionRange.xyz;
        float3 toLight = lightPos - worldPos;
        float dist = length(toLight);

        if (dist > pointLights[i].positionRange.w || dist < 1e-4)
            continue;

        float3 L = toLight / dist;
        if (dot(normal, L) <= 0.0f)
        {
            vis[i] = 0.0f;
            continue;
        }

        float3 up = abs(L.y) < 0.99f ? float3(0, 1, 0) : float3(1, 0, 0);
        float3 tangent = normalize(cross(up, L));
        float3 biTangent = cross(L, tangent);

        float lit = 0.0f;
        for (int s = 0; s < 4; ++s)
        {
            // Spiral disk pattern, rotated per pixel
            float r = sqrt((s + 0.5f) / 4);
            float phi = angle0 + s * 2.3999632f; // golden angle
            float2 d = r * float2(cos(phi), sin(phi));

            float3 target = lightPos + (tangent * d.x + biTangent * d.y) * 1.5f;
            float3 dir = target - origin;
            float tDist = length(dir);

            lit += Occlusion(origin, dir / tDist, tDist);
        }

        vis[i] = lit / 4;
    }

    outputTexture[coords] = vis;
}

[shader("miss")]
void miss(inout ShadowPayload payload) {
    payload.hit = false;
}