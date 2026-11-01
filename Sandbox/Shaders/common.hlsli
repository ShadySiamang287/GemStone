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
};

struct PatchTess {
    float EdgeTess[3] : SV_TessFactor;
    float InsideTess : SV_InsideTessFactor;
};