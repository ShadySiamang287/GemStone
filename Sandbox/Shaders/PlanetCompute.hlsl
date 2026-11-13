#include "common.hlsli"
#include "noise.hlsli"

RWStructuredBuffer<VertexInput> vertexBuffer : register(u0);
StructuredBuffer<PlanetData> planetDataBuffer : register(t0);

// cbuffer PlanetData : register(b1)
// {
// 	float radius;
// 	float roughness;
// 	float ridgeRoughness;
// 	float persistence;
// 	float terrainStrength;
// 	uint vertexCount;
// }

float3 SurfacePoint(PlanetData planet,float3 dir)
{
	// Must be the same expression your vertex displacement uses.
	return dir * (1.0f + FractalNoise(dir, planet.roughness, planet.ridgeRoughness, planet.persistence));
}

float3 CalculateNormal(PlanetData planet, float3 position)
{
	const float eps = 0.01f;

	float3 dir = normalize(position);
	float3 up = abs(dir.y) < 0.99f ? float3(0, 1, 0) : float3(1, 0, 0);
	float3 T = normalize(cross(up, dir));
	float3 B = cross(dir, T);

	float3 p0 = SurfacePoint(planet, dir);
	float3 pT = SurfacePoint(planet, normalize(dir + T * eps));
	float3 pB = SurfacePoint(planet, normalize(dir + B * eps));

	return normalize(cross(pT - p0, pB - p0));
}

float ShapeTerrain(float rawHeight)
{
	float oceanMask = smoothstep(-0.4f, 0.0f, rawHeight);
	float mountainMask = smoothstep(0.2f, 0.6f, rawHeight);

	float oceanVal = rawHeight * 1.4f;
	float plainsVal = rawHeight * 0.7f;
	float mountainVal = 0.14f + pow(max(rawHeight - 0.2f, 0.0f), 1.4f) * 2.2f;

	float heightOutput = lerp(oceanVal, plainsVal, oceanMask);
	heightOutput = lerp(heightOutput, mountainVal, mountainMask);

	return heightOutput;
}

[numthreads(256, 1, 1)]
void PlanetCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint planetID = dispatchThreadID.y;

	PlanetData planet = planetDataBuffer[planetID];
    uint localVertID = dispatchThreadID.x;
    if (localVertID >= planet.vertexCount)
    {
        return;
    }
    uint vertID = localVertID + planetID * planet.vertexCount;
	
	VertexInput vert = vertexBuffer[vertID];
	float3 direction = normalize(vert.position);
	float noise = FractalNoise(direction + planet.position, planet.roughness, planet.ridgeRoughness, planet.persistence);
	float totalNoise = ShapeTerrain(noise) * planet.terrainStrength;
	float displacedRadius = max(planet.radius, planet.radius + totalNoise);
	float3 displacedPosition = direction * displacedRadius;
	vert.position = displacedPosition;
	vert.normal = CalculateNormal(planet, displacedPosition);
	vert.position = normalize(vert.position) * displacedRadius;	
	vertexBuffer[vertID] = vert;
}