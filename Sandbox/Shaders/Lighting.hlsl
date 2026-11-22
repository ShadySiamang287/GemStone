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

Texture2D albedo : register(t0);
Texture2D normal : register(t1);
Texture2D depth : register(t2);
StructuredBuffer<PointLight> pointLights : register(t3);
Texture2D shadowMap : register(t4);

SamplerState LinearSampler : register(s0);

float3 ReconstructWorldPosition(float2 texCoord, float depth) {
    float2 ndc = texCoord * 2.0 - 1.0;
    ndc.y = 1.0f - texCoord.y * 2.0f;
    float4 clipSpacePosition = float4(ndc, depth, 1.0);

    float4 viewPosition = mul(invProjectionMatrix, clipSpacePosition);
    viewPosition /= viewPosition.w;

    float4 worldPosition = mul(invViewMatrix, viewPosition);
    return worldPosition.xyz;
}

float3 CalculatePointLight(PointLight light, float3 worldPos, float3 normal) {
    float3 position = light.positionRange.xyz;
    float range = light.positionRange.w;
    float3 colour = light.colourIntensity.rgb;
    float intensity = light.colourIntensity.w;

    float3 toLight = position - worldPos;
    float distance = length(toLight);

    if (distance > range) {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float3 lightDir = toLight / max(distance, 0.0001f);
    float NdotL = max(dot(normal, lightDir), 0.0f);

    float rangeFactor = saturate(1.0f - (distance / range));
    rangeFactor *= rangeFactor;

    float attenuation = rangeFactor / max(distance * distance, 0.01f);

    return colour * intensity * NdotL * attenuation;
}

float4 LightingPS(FullScreenVertOut input) : SV_Target0{
    float4 colour = albedo.Sample(LinearSampler, input.uv);
    float4 normalSample = normal.Sample(LinearSampler, input.uv);
    float depthSample = depth.Sample(LinearSampler, input.uv).r;

    float3 worldPos = ReconstructWorldPosition(input.uv, depthSample);
    float3 normal = normalize(normalSample.xyz * 2.0f - 1.0f);

    float3 totalLighting = float3(0.f, 0.f, 0.f);
    for (int i = 0; i < lightCount; ++i) {
        float shadowValue = 1.f;
        
        // it has a shadow
        if (i < 4) {
            shadowValue = shadowMap.Sample(LinearSampler, input.uv)[i];
        }
        PointLight light = pointLights[i];
        float3 lightContribution = CalculatePointLight(light, worldPos, normal);
        totalLighting += lightContribution * shadowValue;
    }

    float3 ambient = ambientLight * ambientStrength;
    float4 output = float4(colour.rgb * (ambient + totalLighting), 1.f);

    return output;
}