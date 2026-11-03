#include "TestScene.h"

#include <numbers>

#include <Graphics/ResourceFactory.h>
#include <Graphics/GraphicsCommands.h>
#include <imgui.h>

TestScene::TestScene() {
    using namespace GEM;

    D3D12_ROOT_PARAMETER param;
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    param.Descriptor.ShaderRegister = 0; // Binds to register(u0)
    param.Descriptor.RegisterSpace = 0;  // Binds to space0
    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


    RootSignatureDesc rootDesc;
    rootDesc.name = L"Planet Compute Desc";
    rootDesc.parameters.push_back(param);
    rootDesc.flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    mRootSig = ResourceFactory::CreateRootSignature(rootDesc);
    mComputePipeline = ResourceFactory::CreateComputePipeline(mRootSig, ShaderBytecode("Shaders/PlanetCS.dxil"), L"Planet Compute");

    auto vertices = GeneratePlanetVertices();
    auto indices = GeneratePlanetIndices();

    Model planet;
    planet.primitives.push_back({
        ResourceFactory::CreateVertexBuffer(vertices),
        ResourceFactory::CreateIndexBuffer(indices)
    });

    mModels.push_back(std::move(planet));
    mCamera.cameraPos = glm::vec3(0.f ,0.f, 30.f);
}

void TestScene::Update(GEM::Input& input, GEM::Renderer& renderer, float dt) {
    mCamera.Update(input, dt);
    renderer.SetCameraMatrix(mCamera.getMatrix());

    if (!mLights.empty())
        renderer.UpdateLights(mLights);
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

void TestScene::OnImGuiRenderer() {
    ImGui::Begin("Lights");
    ImGui::Text("Camera pos, X: %f, Y: %f, Z: %f", mCamera.cameraPos.x, mCamera.cameraPos.y, mCamera.cameraPos.z);
    if (ImGui::Button("Add Light")) {
        mLights.push_back({});
    }
    for (int i = 0; i < mLights.size(); i++) {
        ImGui::PushID(i);
        ImGui::Separator();
        ImGui::Text("Light %i", i);
        ImGui::SliderFloat3("Light Position", &mLights[i].positionRange.r, -50.f, 50.f);
        ImGui::SliderFloat("Light Range", &mLights[i].positionRange.a, 0.f, 100.f);
        ImGui::ColorEdit3("Light Colour", &mLights[i].colourIntensity.x);
        ImGui::SliderFloat("Light Intensity", &mLights[i].colourIntensity.w, 0.f, 100.f);
        ImGui::PopID();
    }
    ImGui::End();
}