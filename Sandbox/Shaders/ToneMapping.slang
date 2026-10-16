Texture2D SceneTexture : register(t0);
SamplerState LinearSampler : register(s0);
enum class Type : uint16_t {
    ReinhardLuminance,
    AECS,
    Uncharted2,
    AGX
}

cbuffer Data : register(b0)
{
    float exposure;
    float hableValue;
    Type type;
}

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

float3 ACESFilm(float3 colour) {
    // ACES Filmic Tone Mapping Curve
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;

    return saturate((colour * (a * colour + b)) / (colour * (c * colour + d) + e));
}

// used by uncharted 2 but also in a technique called Hable tonemapping
float3 Uncharted2Tonemap(float3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;

    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 agxContrastApprox(float3 colour) {
    float3 x2 = colour * colour;
    float3 x3 = x2 * colour;
    float3 x4 = x3 * colour;
    float3 x5 = x4 * colour;
    float3 x6 = x5 * colour;

    return +15.5 * x6
           - 40.14 * x5
           + 31.96 * x4
           - 6.86 * x3
           + 0.429 * x2
           + 0.1344 * colour
           + 0.00263;
}

float3 agx(float3 colour) {
    const float3x3 AGX_INPUT_MATRIX = float3x3(
        0.8424202359, 0.0778329080, 0.0797468561,
        0.0457519105, 0.8764617027, 0.0777863868,
        0.0423340051, 0.0760447366, 0.8816212583
    );

    const float3x3 AGX_OUTPUT_MATRIX = float3x3(
        1.1968219602, -0.0980208151, -0.0987513361,
        -0.0528964563, 1.1519782559, -0.0989613143,
        -0.0536723049, -0.0980514151, 1.1517228802
    );

    const float minEv = -12.47393;
    const float maxEv = 4.026069;

    // Log color space framing
    colour = clamp(colour, 0.0, 65504.0); // Safe HDR clamp
    colour = mul(AGX_INPUT_MATRIX, colour);

    colour = log2(colour + 1e-10);
    colour = (colour - minEv) / (maxEv - minEv);
    colour = clamp(colour, 0.0, 1.0);

    colour = agxContrastApprox(colour);
    colour = mul(AGX_OUTPUT_MATRIX, colour);

    return colour;
}

[shader("fragment")]
float4 ToneMappingPS(VSInput input) : SV_Target {
    float3 hdrColor = SceneTexture.Sample(LinearSampler, input.uv).rgb;
    float3 combinedColour = hdrColor * exposure;

    float3 toneMappedColour;
    switch (type) {
        case Type::ReinhardLuminance:
            toneMappedColour = ReinhardLuminance(combinedColour);
            break;
        case Type::AECS:
            // Implement AECS tone mapping here
            toneMappedColour = ACESFilm(combinedColour);
            break;
        case Type::Uncharted2:
            // Implement Uncharted 2 tone mapping here
            float3 current = Uncharted2Tonemap(combinedColour);
            float3 whiteScale = 1.0 / Uncharted2Tonemap(float3(hableValue));
            toneMappedColour = current * whiteScale;
            break;
        case Type::AGX:
            toneMappedColour = agx(combinedColour);
            break;
    }
    return float4(toneMappedColour, 1.0);
}
