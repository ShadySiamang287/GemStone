#include "noise.hlsli"

struct VertexInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    // float4 colour : COLOR0;
};

struct VertexOutput {
    float3 position : POSITION;
    float3 normal : NORMAL;

    // float terrainHeight : TEXCOORD0;
    // float3 direction : TEXCOORD1;
};

cbuffer FrameData : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
}

static const float radius = 10.f;
static const float amplitude = 0.5f;
static const float frequency = 4.0f;
static const float seed = 0.0f;
static const float terrainStrength = 2.0f;

VertexOutput vertMain(VertexInput input) {
    VertexOutput output;

    // float3 direction = normalize(input.position);
    // float noiseValue = FractalNoise(direction, frequency, amplitude);

    // float displacedRadius = radius + noiseValue * terrainStrength;
    // float3 displacedPosition = direction * displacedRadius;
    // float terrainHeight = saturate(noiseValue * 0.5f + 0.5f);

    output.position = input.position;
    //output.sv_position = mul(ProjectionMatrix, mul(ViewMatrix, float4(displacedPosition, 1.0)));
    output.normal = input.normal;
    // output.terrainHeight = terrainHeight;
    // output.direction = direction;
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

    float3 normal = normalize(inVert.normal);
    float3 up = float3(0.0f, 1.0f, 0.0f);

    // float slope = 1.0f - abs(dot(normal, up));
    float latitude = abs(inVert.direction.y);

    float landMask = smoothstep(0.35f, 0.42f, height);

    float grassMask = smoothstep(0.42f, 0.55f, height);

    float rockHeightMask = smoothstep(0.58f, 0.72f, height);

    float steepRockMask = smoothstep(0.35f, 0.75f, height);

    float coldLatitudeMask = smoothstep(0.55f, 0.90f, latitude);

    float snowMask = max(rockHeightMask, coldLatitudeMask);

    float3 colour = float3(0.02f, 0.15f, 0.45f); // ocean

    colour = lerp(
        colour,
        float3(0.75f, 0.62f, 0.35f), // sand
        landMask
    );

    colour = lerp(
        colour,
        float3(0.12f, 0.42f, 0.12f), // grass
        grassMask
    );

    colour = lerp(
        colour,
        float3(0.30f, 0.28f, 0.24f), // rock
        max(steepRockMask, rockHeightMask)
    );

    colour = lerp(
        colour,
        float3(0.95f, 0.95f, 1.0f), // snow
        snowMask
    );

    FragOut output;

    output.colour = float4(colour, 1.0);
    float3 encodedNormal = normal * 0.5f + 0.5; 
    output.normal = float4(encodedNormal, 1.0);
    return output;
}