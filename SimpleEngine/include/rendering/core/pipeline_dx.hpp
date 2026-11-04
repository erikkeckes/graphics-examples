#pragma once
#include "rendering/common_dx.hpp"
#include <unordered_map>

namespace SE
{
class Shader;
class ShaderParamContainer;
class RootSignature;
class PipelineStateObject;

class Pipeline
{
public:
    static std::shared_ptr<Pipeline> GetPipeline(const GraphicsPipelineDesc& desc);
    static std::shared_ptr<Pipeline> GetPipeline(const ComputePipelineDesc& desc);

    Pipeline() = delete;
    ~Pipeline();

private:
    friend class CommandList;
    friend class Device;
    Pipeline(const GraphicsPipelineDesc& desc);
    Pipeline(const ComputePipelineDesc& desc);

    static bool CheckCache(uint64_t hash);

    void AddParams(const Shader& shader);

    bool m_compute = false;
    PipelineSettings m_settings;
    GraphicsShaderState m_graphicShaders;
    ComputeShaderState m_computeShaders;

    std::shared_ptr<RootSignature> m_signature;
    std::shared_ptr<PipelineStateObject> m_pso;
    ShaderParamContainer* m_shaderBinds = nullptr;

    static std::unordered_map<uint64_t, std::shared_ptr<Pipeline>> m_pipelineCache;
};
}  // namespace SE