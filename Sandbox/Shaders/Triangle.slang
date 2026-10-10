struct VertexInput {
    float3 position : POSITION;
    float4 colour : COLOR0;
}

struct VertexOutput {
    float4 sv_position : SV_Position;
    float3 color : COLOR0;
};

[shader("vertex")]
VertexOutput vertMain(VertexInput input) {
    VertexOutput output;
    output.sv_position = float4(input.position, 1.0);
    output.color = input.colour.rgb;
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target
{
    float3 color = inVert.color;
    return float4(color, 1.0);
}