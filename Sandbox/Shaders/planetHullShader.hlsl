
struct VertexOut {
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct HullOut {
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct PatchTess {
    float EdgeTess[3] : SV_TessFactor;
    float InsideTess : SV_InsideTessFactor;
};

static const float maxTess = 8.0f;
static const float nearDist = 15.0f;
static const float farDist = 60.0f;

cbuffer FrameData : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
}

float CalcTessFactor(float3 a, float3 b)
{
    float3 mid = 0.5f * (a + b);
    float dist = length(mul(ViewMatrix, float4(mid, 1.0f)).xyz);
    float t = saturate((dist - nearDist) / (farDist - nearDist));
    return lerp(maxTess, 1.0f, t);
}

PatchTess PatchConstantFunc(InputPatch<VertexOut, 3> patch, uint patchID: SV_PrimitiveID) {
    PatchTess tess;
    
    tess.EdgeTess[0] = CalcTessFactor(patch[1].position, patch[2].position);
    tess.EdgeTess[1] = CalcTessFactor(patch[2].position, patch[0].position);
    tess.EdgeTess[2] = CalcTessFactor(patch[0].position, patch[1].position);
    
    tess.InsideTess = (tess.EdgeTess[0] + tess.EdgeTess[1] + tess.EdgeTess[2]) / 3.0f;
    
    return tess;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchConstantFunc")]
HullOut PlanetHullShader(InputPatch<VertexOut, 3> patch, uint i : SV_OutputControlPointID, uint patchID : SV_PrimitiveID) {
    HullOut hOut;
    hOut.position = patch[i].position;
    hOut.normal = patch[i].normal;
    return hOut;
}