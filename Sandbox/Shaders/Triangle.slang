struct VertexInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 colour : COLOR0;
};

struct VertexOutput {
    float4 sv_position : SV_Position;
    float3 normal : NORMAL;
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
    output.sv_position = mul(ProjectionMatrix, mul(ViewMatrix, float4(input.position, 1.0)));
    output.color = input.colour;
    output.normal = input.normal;
    return output;
}

struct FragOut {
    float4 colour : SV_Target0;
    float4 normal : SV_Target1;
}

[shader("fragment")]
FragOut fragMain(VertexOutput inVert)
{
    FragOut output;

    output.colour = float4(inVert.color.rgb, 1.0);
    output.normal = float4(inVert.normal, 1.0);
    return output;
}