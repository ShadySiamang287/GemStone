#include "TestScene.h"

#include <numbers>

#include <Graphics/ResourceFactory.h>

TestScene::TestScene() {

    auto vertices = GeneratePlanetVertices();
    auto indices = GeneratePlanetIndices();

    GEM::Model planet;
    planet.primitives.push_back({
        GEM::ResourceFactory::CreateVertexBuffer(vertices),
        GEM::ResourceFactory::CreateIndexBuffer(indices)
    });

    mModels.push_back(std::move(planet));
    mCamera.cameraPos = glm::vec3(0.f ,0.f, 30.f);
}

void TestScene::Update(GEM::Input& input, GEM::Renderer& renderer, float dt) {
    mCamera.Update(input, dt);
    renderer.SetCameraMatrix(mCamera.getMatrix());

}

std::vector<GEM::Vertex> TestScene::GeneratePlanetVertices(){
    constexpr float radius = 1.f;
    constexpr float inverseRad = 1.f / radius;
    constexpr int stackCount = 20;
    constexpr int sectorCount = 20;

    constexpr float sectorStep = (2 * std::numbers::pi_v<float>) / static_cast<float>(sectorCount);
    constexpr float stackStep = std::numbers::pi_v<float> / static_cast<float>(stackCount);

    std::vector<GEM::Vertex> out;
    out.reserve(stackCount * sectorCount);

    for (int i = 0; i <= stackCount; i++) {
        float stackAngle = (std::numbers::pi_v<float> / 2.f) - i * stackStep;
        float xy = radius * std::cosf(stackAngle);
        float z = radius * std::sinf(stackAngle);

        for (int j = 0; j <= sectorCount; j++) {
            GEM::Vertex vert;
            float sectorAngle = j * sectorStep;
            vert.position = {
                xy * std::cosf(sectorAngle),
                xy * std::sinf(sectorAngle),
                z
            };

            vert.normal = {
                vert.position.x * inverseRad,
                vert.position.y * inverseRad,
                vert.position.z * inverseRad,
            };

            out.push_back(vert);
        }
    }
    return out;
}
std::vector<uint32_t> TestScene::GeneratePlanetIndices(){
    constexpr int stackCount = 20;
    constexpr int sectorCount = 20;

    std::vector<uint32_t> indices;

    for (int i = 0; i < stackCount; i++) {
        int k1 = i * (sectorCount + 1);
        int k2 = k1 + sectorCount + 1;

        for(int j = 0; j < sectorCount; j++, k1++, k2++) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if(i != (stackCount-1))
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }

        }
    }
    std::reverse(indices.begin(), indices.end());
    return indices;
}