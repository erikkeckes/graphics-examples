#pragma once
#include "rendering/common_dx.hpp"
#include <unordered_map>

namespace SE
{
class RootSignature;

class PipelineStateObject
{
public:
    PipelineStateObject() = delete;
    ~PipelineStateObject();

    static std::shared_ptr<PipelineStateObject> GetPSO(const PipelineSettings& settings,
                                                       const std::shared_ptr<RootSignature>& signature,
                                                       const GraphicsShaderState& shaders);

    static std::shared_ptr<PipelineStateObject> GetPSO(const std::shared_ptr<RootSignature>& signature,
                                                       const std::shared_ptr<Shader>& compute);

private:
    friend class Device;
    friend class Pipeline;
    friend class CommandList;
    PipelineStateObject(const PipelineSettings& settings,
                        const std::shared_ptr<RootSignature>& signature,
                        const GraphicsShaderState& shaders);
    PipelineStateObject(const std::shared_ptr<RootSignature>& signature, const std::shared_ptr<Shader>& compute);

    static uint64_t GetHash(const PipelineSettings& settings,
                            const std::shared_ptr<RootSignature>& signature,
                            const GraphicsShaderState& shaders);

    static uint64_t GetHash(const std::shared_ptr<RootSignature>& signature, const std::shared_ptr<Shader>& compute);

    void SaveBinary() const;

    bool LoadBinary(std::vector<char>& binary) const;

    uint64_t m_hash;
    ID3D12PipelineState* m_pso = nullptr;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_graphicsDesc{};
    D3D12_COMPUTE_PIPELINE_STATE_DESC m_computeDesc{};

    static std::unordered_map<uint64_t, std::shared_ptr<PipelineStateObject>> m_psoCache;
};
}  // namespace SE