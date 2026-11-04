#pragma once
#include "core/ecs.hpp"
#include "rendering/common_dx.hpp"
#include <imgui/IconsFontAwesome.h>

namespace SE
{
class Pipeline;
class UploadBuffer;
struct Texture;
struct TextureData;
class Buffer;
class D3D12Texture;
class CommandList;
class Mesh;

class Renderer : public System
{
public:
    Renderer();
    ~Renderer();

    virtual void Render() override;

private:
    void SetTexture(TextureData& data, const std::shared_ptr<Texture>& texture) const;

    static std::string GetTexturePath(const std::shared_ptr<Texture>& texture);

    void CreateDefferedPasses();

    void CreateBuffers();

    void CreateSamplers();

    void GenerateLut();

    void CheckGbuffersResize();

    void ClearGbuffers() const;

    void GPass(const D3D12_GPU_VIRTUAL_ADDRESS& cameraBuff) const;

    void PointLightPreepass(const D3D12_GPU_VIRTUAL_ADDRESS& cameraBuff) const;

    void DirectLight(const D3D12_GPU_VIRTUAL_ADDRESS& invCameraBuff);

    void PointLight(const D3D12_GPU_VIRTUAL_ADDRESS& invCameraBuff, const D3D12_GPU_VIRTUAL_ADDRESS& cameraBuff);

    void IBL(const D3D12_GPU_VIRTUAL_ADDRESS& invCameraBuff);

    void ToneMapingPass() const;

    void Bloom() const;

    void BloomExtract() const;

    void BloomDownSample() const;

    void BloomUpsample() const;

    void RenderDebug(const D3D12_GPU_VIRTUAL_ADDRESS& invCameraBuff) const;

    void SetupCommandListForDefferedLights(CommandList& list, Buffer* vertex, Buffer* index, bool depth) const;

    static PipelineSettings CreateDefferdSettings();

    std::shared_ptr<Pipeline> m_gpassPipeline;

    // light passes
    std::shared_ptr<Pipeline> m_directPipeline;
    std::shared_ptr<Pipeline> m_pointStencilPipeline;
    std::shared_ptr<Pipeline> m_pointPipeline;
    std::shared_ptr<Pipeline> m_iblPipeline;
    std::shared_ptr<Pipeline> m_toneMappingPipeline;

    std::shared_ptr<Pipeline> m_bloomPrepassPipeline;
    std::shared_ptr<Pipeline> m_bloomDownsamplePipeline;
    std::shared_ptr<Pipeline> m_bloomUpsamplePipeline;

    std::shared_ptr<Pipeline> m_debugPipeline;

    UploadBuffer* m_upload = nullptr;

    D3D12Texture* m_lutTexture = nullptr;

    Buffer* m_quadVerticies = nullptr;
    Buffer* m_quadIndecies = nullptr;

    std::array<D3D12Texture*, 3> m_gbuffers;

    D3D12Texture* m_defferedTarget = nullptr;
    D3D12Texture* m_bloomTarget = nullptr;

    std::shared_ptr<Mesh> m_sphere;

    uint32_t m_defaultSampler;
    uint32_t m_bufferSampler;
    uint32_t m_bloomSampler;

    uint32_t m_colorMode = 0;
    uint32_t m_mappingStyle = 2;
    float m_bloomStrength = 0.01f;
    float m_bloomThreshold = 0;
};
}  // namespace SE