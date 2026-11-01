#include "common.hlsli"
struct PointLight{
    float4 positionRange;
    float4 colourIntensity;
};

cbuffer FrameData : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4x4 invViewMatrix;
    float4x4 invProjectionMatrix;
    int lightCount;
    float3 _padding;
}

Texture2D albedo : register(t0);
Texture2D normal : register(t1);
Texture2D depth : register(t2);
StructuredBuffer<PointLight> pointLights : register(t3);
SamplerState LinearSampler : register(s0);

struct FragOut {
    float4 colour : SV_Target0;
    float4 brightness : SV_Target1;
};

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

    // float3 lightDir = normalize(light.positionRange.xyz - worldPos);
    // float distance = length(light.positionRange.xyz - worldPos);
    // float attenuation = 1.0f / (distance * distance);
    // float NdotL = max(dot(normal, lightDir), 0.0f);
    // return light.colourIntensity.rgb * NdotL * attenuation;
}

FragOut LightingPS(FullScreenVertOut input) {
    FragOut output;
    float4 colour = albedo.Sample(LinearSampler, input.uv);
    float4 normalSample = normal.Sample(LinearSampler, input.uv);
    float depthSample = depth.Sample(LinearSampler, input.uv).r;

    for (int i = 0; i < lightCount; ++i) {
        PointLight light = pointLights[i];
        float3 worldPos = ReconstructWorldPosition(input.uv, depthSample);
        float3 normal = normalize(normalSample.xyz * 2.0f - 1.0f);
        float3 lightContribution = CalculatePointLight(light, worldPos, normal);
        colour.rgb += lightContribution;
    }


    float brightness = dot(colour.rgb, float3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0f) {
        output.brightness = float4(colour.rgb, 1.0f);
    } else {
        output.brightness = float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    output.colour = colour;
    return output;
}