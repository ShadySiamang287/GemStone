#pragma once

namespace GEM{
    class GEM_API ComputePipeline {
    private:
        ComputePipeline() {};
        Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState;
    
        friend class GraphicsCommands;
        friend class ResourceFactory;
    };
}