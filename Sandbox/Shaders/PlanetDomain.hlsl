#include "noise.hlsli"
#include "common.hlsli"

struct HullOut {
    float3 PosL : POSITION;
    float3 normal : NORMAL;
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
    float4x4 invViewMatrix;
    float4x4 invProjectionMatrix;
    int lightCount;
    float3 _padding;
}

cbuffer PlanetData : register(b1)
{
    float radius;
    float roughness;
    float ridgeRoughness;
    float persistence;
    float terrainStrength;
}

float3 SurfacePoint(float3 dir)
{
    // Must be the same expression your vertex displacement uses.
    return dir * (1.0f + FractalNoise(dir, roughness, ridgeRoughness, persistence));
}

float3 CalculateNormal(float3 position)
{
    const float eps = 0.01f;

    float3 dir = normalize(position);
    float3 up = abs(dir.y) < 0.99f ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 T = normalize(cross(up, dir));
    float3 B = cross(dir, T);

    float3 p0 = SurfacePoint(dir);
    float3 pT = SurfacePoint(normalize(dir + T * eps));
    float3 pB = SurfacePoint(normalize(dir + B * eps));

    return normalize(cross(pT - p0, pB - p0));
}

float ShapeTerrain(float rawHeight) {
    float oceanMask = smoothstep(-0.4f, 0.0f, rawHeight);
    float mountainMask = smoothstep(0.2f, 0.6f, rawHeight);

    float oceanVal = rawHeight * 1.4f;
    float plainsVal = rawHeight * 0.7f;
    float mountainVal = 0.14f + pow(max(rawHeight - 0.2f, 0.0f), 1.4f) * 2.2f;

    float heightOutput = lerp(oceanVal, plainsVal, oceanMask);
    heightOutput = lerp(heightOutput, mountainVal, mountainMask);

    return heightOutput;
}

[domain("tri")]
VertexOutDS PlanetDS(PatchTess patchTress, float3 bary: SV_DomainLocation, const OutputPatch<HullOut, 3> tri) {
    VertexOutDS dOut;

    float3 posL = bary.x * tri[0].PosL + bary.y * tri[1].PosL + bary.z * tri[2].PosL;
    float3 normal = bary.x * tri[0].normal + bary.y * tri[1].normal + bary.z * tri[2].normal;
    float3 direction = normalize(posL);
    float noise = FractalNoise(direction, roughness, ridgeRoughness, persistence);
    
    float totalNoise = ShapeTerrain(noise) * terrainStrength;
    
    float displacedRadius = max(radius, radius + totalNoise);
    float3 displacedPosition = posL * displacedRadius;


    dOut.position = mul(ProjectionMatrix, mul(ViewMatrix, float4(displacedPosition, 1.0)));
    dOut.normal = CalculateNormal(displacedPosition);
    dOut.terrainHeight = saturate(noise);
    dOut.direction = direction;
    return dOut;
}