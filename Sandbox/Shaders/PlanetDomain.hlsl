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

cbuffer PlanetData : register(b1)
{
    float radius;
    float roughness;
    float persistence;
    float terrainStrength;
}

float3 CalculateNormal(float3 position)
{
    const float eps = 0.001f;
    const float2 h = float2(eps, 0.0f);
    
    return normalize(float3(
        FractalNoise(position + h.xyy, roughness, persistence).value - FractalNoise(position - h.xyy, roughness, persistence).value,
        FractalNoise(position + h.yxy, roughness, persistence).value - FractalNoise(position - h.yxy, roughness, persistence).value,
        FractalNoise(position + h.yyx, roughness, persistence).value - FractalNoise(position - h.yyx, roughness, persistence).value
    ));
    
    //return normalize(float3(FractalNoise(position - h.xyy, frequency, amplitude).value - FractalNoise(position + h.xyy, frequency, amplitude).value,
    //                        FractalNoise(position - h.yxy, frequency, amplitude).value - FractalNoise(position + h.yxy, frequency, amplitude).value,
    //                        FractalNoise(position - h.yyx, frequency, amplitude).value - FractalNoise(position + h.yyx, frequency, amplitude).value));
    //const float h = 0.0001;
    //const float2 k = float2(1, -1);
    
    //return normalize(
    //    k.xyy * FractalNoise(position + k.xyy * h, frequency, amplitude).value +
    //    k.yyx * FractalNoise(position + k.yyx * h, frequency, amplitude).value +
    //    k.yxy * FractalNoise(position + k.yxy * h, frequency, amplitude).value +
    //    k.xxx * FractalNoise(position + k.xxx * h, frequency, amplitude).value
    //);
    
    //const float eps = 0.0001f;
    //const float2 h = float2(eps, 0.0f);
    //return normalize(float3(
    //    FractalNoise(position + h.xyy, frequency, amplitude).value - FractalNoise(position - h.xyy, frequency, amplitude).value,
    //    FractalNoise(position + h.yxy, frequency, amplitude).value - FractalNoise(position - h.yxy, frequency, amplitude).value,
    //    FractalNoise(position + h.yyx, frequency, amplitude).value - FractalNoise(position - h.yyx, frequency, amplitude).value
    //));
}

[domain("tri")]
VertexOutDS PlanetDS(PatchTess patchTress, float3 bary: SV_DomainLocation, const OutputPatch<HullOut, 3> tri) {
    VertexOutDS dOut;

    float3 posL = bary.x * tri[0].PosL + bary.y * tri[1].PosL + bary.z * tri[2].PosL;
    float3 normal = bary.x * tri[0].normal + bary.y * tri[1].normal + bary.z * tri[2].normal;
    float3 direction = normalize(posL);
    NoiseResult noise = FractalNoise(direction, roughness, persistence);
    
    float totalNoise = noise.value * terrainStrength;
    
    float displacedRadius = radius + totalNoise;
    float3 displacedPosition = posL * displacedRadius;
    
    float3 radiusGradient = noise.gradient * terrainStrength;
    float3 surfaceGradient = radiusGradient - direction * dot(radiusGradient, direction);

    dOut.position = mul(ProjectionMatrix, mul(ViewMatrix, float4(displacedPosition, 1.0)));
    dOut.normal = CalculateNormal(displacedPosition);
    dOut.terrainHeight = saturate(noise.value * 0.5f + 0.5f);
    dOut.direction = direction;
    return dOut;
}