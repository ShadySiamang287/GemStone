Texture2D SceneTexture : register(t0);
SamplerState LinearSampler : register(s0);

struct VSInput {
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

float3 ReinhardLuminance(float3 color)
{
    float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));

    float mappedLuminance = luminance / (1.0 + luminance);

    return color * (mappedLuminance / max(luminance, 0.0001));
}

[shader("fragment")]
float4 ToneMappingPS(VSInput input) : SV_Target {
    float3 hdrColor = SceneTexture.Sample(LinearSampler, input.uv).rgb;
    float exposure = 1.0;
    float3 combinedColour = hdrColor * exposure;
    float3 toneMappedColour = ReinhardLuminance(combinedColour);
    float3 sRGBColour = pow(toneMappedColour, float3(1.0 / 2.2));
    return float4(toneMappedColour, 1.0);
}
