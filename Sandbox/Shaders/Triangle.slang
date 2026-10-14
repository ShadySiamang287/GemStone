struct VertexInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 colour : COLOR0;
};

struct VertexOutput {
    float4 sv_position : SV_Position;
    float3 worldPosition : TEXCOORD0;
    float4 color : COLOR0;
};

cbuffer FrameData : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
}

[shader("vertex")]
VertexOutput vertMain(VertexInput input) {
    VertexOutput output;
    output.worldPosition = input.position;
    output.sv_position = mul(ProjectionMatrix, mul(ViewMatrix, float4(input.position, 1.0)));
    output.color = input.colour;
    return output;
}


[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target
{
    float3 color = inVert.color.rgb;
    return float4(color, 1.0);
}