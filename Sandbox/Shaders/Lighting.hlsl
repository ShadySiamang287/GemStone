Texture2D albedo : register(t0);
Texture2D normal : register(t1);
Texture2D depth : register(t2);
SamplerState LinearSampler : register(s0);

struct VSInput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

struct FragOut {
    float4 colour : SV_Target0;
    float4 brightness : SV_Target1;
};

FragOut LightingPS(VSInput input) {
    FragOut output;
    float4 colour = albedo.Sample(LinearSampler, input.uv);
    float brightness = dot(colour.rgb, float3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0f) {
        output.brightness = float4(colour.rgb, 1.0f);
    } else {
        output.brightness = float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    output.colour = colour;
    return output;
}