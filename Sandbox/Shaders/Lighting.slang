Texture2D albedo : register(t0);
Texture2D normal : register(t1);
Texture2D depth : register(t2);
SamplerState LinearSampler : register(s0);

struct VSInput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

[shader("fragment")]
float4 LightingPS(VSInput input) : SV_Target {
    float3 colour = albedo.Sample(LinearSampler, input.uv).rgb;
    return float4(colour, 1.0f);
}