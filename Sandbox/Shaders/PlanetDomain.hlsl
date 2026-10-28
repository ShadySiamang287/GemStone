#include "noise.hlsli"

struct HullOut {
    float3 PosL : POSITION;
    float3 normal : NORMAL;
};

struct PatchTess {
    float EdgeTess[3] : SV_TessFactor;
    float InsideTess : SV_InsideTessFactor;
};

struct VertexOutDS {
    float4 position : SV_Position;
    float3 normal : NORMAL;

    float terrainHeight : TEXCOORD0;
    float3 direction : TEXCOORD1;
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

[domain("tri")]
VertexOutDS PlanetDS(PatchTess patchTress, float3 bary: SV_DomainLocation, const OutputPatch<HullOut, 3> tri) {
    VertexOutDS dOut;

    float3 posL = bary.x * tri[0].PosL + bary.y * tri[1].PosL + bary.z * tri[2].PosL;
    float3 normal = bary.x * tri[0].normal + bary.y * tri[1].normal + bary.z * tri[2].normal;
    float3 direction = normalize(posL);
    NoiseResult noise = FractalNoise(direction, frequency, amplitude);

    float displacedRadius = radius + noise.value * terrainStrength;
    float3 displacedPosition = posL * displacedRadius;
    
    float3 radiusGradient = noise.gradient * terrainStrength;
    float3 surfaceGradient = radiusGradient - direction * dot(radiusGradient, direction);

    dOut.position = mul(ProjectionMatrix, mul(ViewMatrix, float4(displacedPosition, 1.0)));
    dOut.normal = normalize(displacedRadius * direction - surfaceGradient);
    dOut.terrainHeight = saturate(noise.value * 0.5f + 0.5f);
    dOut.direction = direction;
    return dOut;
}