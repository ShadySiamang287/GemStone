#pragma once

#include <Renderer/RenderPasses/RenderPass.h>
#include <Graphics/ResourceFactory.h>



struct BloomData{
    bool horizontal = true;
};

class BloomPass : public GEM::RenderPass{
public:
    BloomPass();
    void Render(GEM::RenderTargetManager& manager, std::span<GEM::Model> models) override;

    void OnImGuiRender() override;
private:
    std::shared_ptr<GEM::RootSignature> mRootSig; 
    std::unique_ptr<GEM::GraphicsPipeline> mPipeline;
    std::unique_ptr<GEM::GraphicsPipeline> mBrightnessPipeline;
    BloomData mData;
    std::unique_ptr<GEM::ConstantBuffer<BloomData>> mConstantBuffer;
};