RaytracingAccelerationStructure accelStructure : register(t0);
RWTexture2D<float4> outputTexture : register(u0);

[shader("raygeneration")]
void rayGen() {
    uint3 launchIndex = DispatchRaysIndex();
    float3 col = float3(0.4, 0.6, 0.2);
    outputTexture[launchIndex.xy] = float4(col, 1.0);
}

struct Payload {
    bool hit;
};

[shader("miss")]
void miss(inout Payload payload) {
    payload.hit = false;
}

[shader("closesthit")]
void closestHit(inout Payload payload, in BuiltInTriangleIntersectionAttributes attr) {
    payload.hit = true;
}