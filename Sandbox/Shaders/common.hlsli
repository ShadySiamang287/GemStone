struct PointLight{
    float4 positionRange;
    float4 colourIntensity;
};

struct FullScreenVertOut {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

struct VertexInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct VertexOut {
    float3 position : POSITION;
    float3 normal : NORMAL;
    nointerpolation uint instanceID : TEXCOORD0;
};

struct PatchTess {
    float EdgeTess[3] : SV_TessFactor;
    float InsideTess : SV_InsideTessFactor;
};

struct PlanetData {
    float3 position;
    float radius;
    float roughness;
    float ridgeRoughness;
    float persistence;
    float terrainStrength; 
    uint vertexCount;
};