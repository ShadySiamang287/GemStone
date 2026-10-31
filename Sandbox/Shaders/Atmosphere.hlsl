cbuffer FrameData : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4x4 invViewMatrix;
    float4x4 invProjectionMatrix;
}

struct VertexInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct VertexOutput {
    float4 position : SV_Position;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
};

VertexOutput atmosphereVS(VertexInput vertIn) {
    VertexOutput vert;
    float3 scaledPosition = normalize(vertIn.position) * 11.25;
    vert.position = mul(ProjectionMatrix, mul(ViewMatrix, float4(scaledPosition, 1.0)));
    vert.worldPos = scaledPosition;
    vert.normal = normalize(vertIn.normal);

    return vert;
}

float4 atmospherePS(VertexOutput vertIn) : SV_Target0{
    float3 normal = vertIn.normal;
    float3 cameraPosition = mul(
                                invViewMatrix,
                                float4(0, 0, 0, 1)
    ) .xyz;

    float3 viewDir = normalize(
        cameraPosition - vertIn.worldPos
    );

    float NdotV = saturate(dot(normal, viewDir));

    float fresnel = 1.0 - saturate(NdotV);

    float atmosphere = pow(fresnel, 1.25);

    float3 colour = float3(0.15, 0.4, 1.0);

    return float4(colour, atmosphere * 0.8);
}