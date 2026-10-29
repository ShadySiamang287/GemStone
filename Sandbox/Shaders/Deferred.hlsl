#include "noise.hlsli"

struct VertexInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct VertexOutput {
    float3 position : POSITION;
    float3 normal : NORMAL;
};

cbuffer FrameData : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
}

VertexOutput vertMain(VertexInput input) {
    VertexOutput output;

    output.position = input.position;
    output.normal = input.normal;

    return output;
}

struct FragOut {
    float4 colour : SV_Target0;
    float4 normal : SV_Target1;
};

struct VertexOutDS {
    float4 position : SV_Position;
    float3 normal : NORMAL;

    float terrainHeight : TEXCOORD0;
    float3 direction : TEXCOORD1;
};

FragOut fragMain(VertexOutDS inVert)
{
    float height = inVert.terrainHeight;
    const float seaLevel = 0.34f;

    float3 normal = normalize(inVert.normal);
    float3 up = float3(inVert.direction);
        
    float slope = 1.0f - abs(dot(normal, up));
    float latitude = abs(inVert.direction.y);

    float n = SimplexNoise(up * 40.0f) - 0.5f;
    float h = height + n * 0.03f;
    float s = slope  + n * 0.10f;


    float landMask  = smoothstep(0.15f, 0.23f, height);
    float grassMask = smoothstep(0.25f, 0.8f, height);

    float rockHeightMask = smoothstep(0.8f, 0.85f, height);
    float steepRockMask  = smoothstep(0.55f, 0.65f, slope);
    float finalRock      = smoothstep(0.75f, 0.90f, slope);

    float coldLatitudeMask = smoothstep(0.75f, 0.92f, latitude);
    float highAltitudeSnow = smoothstep(0.85f, 0.95f, height);
    float snowMask         = max(highAltitudeSnow, coldLatitudeMask);

    float dirtMask = smoothstep(0.55f, 0.72f, height) * (1.0f - finalRock);
    
    // Define Biome Colours
    float3 oceanColor = float3(0.02f, 0.15f, 0.45f);
    float3 sandColor  = float3(0.75f, 0.62f, 0.35f);
    float3 grassColor = float3(0.15f, 0.52f, 0.18f);
    float3 rockColor  = float3(0.42f, 0.42f, 0.46f); // Cool grey/slate
    float3 snowColor  = float3(0.95f, 0.95f, 1.0f);
    float3 dirtColor = float3(0.50f, 0.38f, 0.25f);

    // Masks (all relative to sea level)
    float beach = 1.0f - smoothstep(seaLevel + 0.01f, seaLevel + 0.06f, h);
    float grass = smoothstep(seaLevel + 0.03f, seaLevel + 0.12f, h);
    float dirt  = smoothstep(0.55f, 0.72f, h);
    float rock  = max(smoothstep(0.55f, 0.75f, s), smoothstep(0.80f, 0.90f, h));
    float snow  = max(smoothstep(0.85f, 0.95f, h),
                      smoothstep(0.75f, 0.92f, latitude + n * 0.1f));
    snow *= 1.0f - smoothstep(0.45f, 0.65f, s); 

    float3 land = grassColor;
    land = lerp(land, dirtColor, dirt * (1.0f - rock));
    land = lerp(land, rockColor, rock);
    land = lerp(land, snowColor, snow);
    land = lerp(land, sandColor, beach);
    
    float depth = saturate((seaLevel - height) / 0.2f);
    float3 water = lerp(float3(0.05, 0.45, 0.55), float3(0.01, 0.06, 0.25), sqrt(depth));

    float isLand = smoothstep(seaLevel - 0.005f, seaLevel + 0.005f, height);
    float3 colour = lerp(water, land, isLand);

    colour *= lerp(0.7f, 1.0f, pow(saturate(dot(normal, up)), 2.0f));

    FragOut output;

    output.colour = float4(colour, 1.0);
    float3 encodedNormal = normal * 0.5f + 0.5; 
    output.normal = float4(encodedNormal, 1.0);
    return output;
}