#pragma once

#include "Graphics/GraphicsPipeline.h"
#include "Graphics/RootSignature.h"
#include "Graphics/GPUResource.h"

namespace GEM{
    struct LocalRootSigAssociation{
        std::shared_ptr<RootSignature> rootSignature;
        std::vector<std::wstring> exports;
    };
    
    struct RTPipelineDesc{
        std::vector<ShaderBytecode> libraryBytecodes;
        std::vector<D3D12_HIT_GROUP_DESC> hitGroups;
        std::shared_ptr<RootSignature> globalRootSig;
        std::vector<LocalRootSigAssociation> localRootSigs;
        UINT maxRecursionDepth = 0;
    };

    // uhh i have no idea what needs to go in here );
    struct HitPayload{
        float tHit;
    };

    class RaytracingPipeline{
    private:
        RaytracingPipeline() {}
        std::shared_ptr<RootSignature> mGlobalRootSig;
        std::vector<LocalRootSigAssociation> mLocalRootSigs;

        Microsoft::WRL::ComPtr<ID3D12StateObject> mStateObject;

        friend class ResourceFactory;
        friend class GraphicsCommands;
    };

    struct ShaderRecordDesc{
        std::wstring exportName;
        std::vector<uint8_t> localArgs;
    };
    
    struct TableLayout { 
        UINT offset = 0;
        UINT size = 0;
        UINT stride = 0;
        UINT count = 0;
    };

    struct ShaderTableDesc{
        RaytracingPipeline* pipeline;
        ShaderRecordDesc rayGen;
        std::vector<ShaderRecordDesc> missShaders;
        std::vector<ShaderRecordDesc> hitGroups;
    };

    class ShaderTable : public GPUBuffer{
    public:
        void FillDispatchDesc(D3D12_DISPATCH_RAYS_DESC& desc);
    private:
        ShaderTable() {}

        TableLayout mRayGen, mMiss, mHit;

        friend class ResourceFactory;
        friend class GraphicsCommands;
    };
}