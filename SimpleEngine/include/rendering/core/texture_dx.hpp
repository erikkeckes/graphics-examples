#pragma once
#include "core/resource.hpp"
#include "rendering/common_dx.hpp"
#include "glm/glm.hpp"
#include <vector>

namespace SE
{
class D3D12Resource;
class ShaderResourceView;
class UnorderedAccessView;
class RenderTargetView;
class DepthStencilView;
class CommandList;

class D3D12Texture
{
public:
    // creating texture for a specific reason in rendering pipeline
    D3D12Texture() = delete;
    D3D12Texture(TextureDesc desc);
    // texture from swapchain
    D3D12Texture(TextureDesc desc, IDXGISwapChain2* swapchain, uint32_t index);
    ~D3D12Texture();

    DELETEMOVECOPY(D3D12Texture);

    static uint32_t GetMaxMips(uint32_t width, uint32_t height);

    const ShaderResourceView& GetSRV() const;
    const ShaderResourceView& GetSRV(int index) const;
    const UnorderedAccessView& GetUAV() const;
    const UnorderedAccessView& GetUAV(int index) const;
    const RenderTargetView& GetRTV() const;
    const RenderTargetView& GetRTV(int index) const;
    const DepthStencilView& GetDSV() const;

    const TextureDesc& GetDesc() const { return m_desc; }

    glm::vec4 GetClearColor() const { return m_clearColor; }

    std::string GetName() const { return m_desc.debugName; }

    void Transition(CommandList& list, D3D12_RESOURCE_STATES state, uint32_t subresource = UINT32_MAX) const;

private:
    void UploadData();

    void GenerateMipsTexture2D();
    void GenerateMipsTexture2DArray();
    friend class CommandList;
    friend class Skybox;

    DepthStencilView* m_dsv = nullptr;

    std::vector<ShaderResourceView*> m_srvs;
    std::vector<RenderTargetView*> m_rtvs;
    std::vector<UnorderedAccessView*> m_uavs;

    TextureDesc m_desc;

    glm::vec4 m_clearColor = {1, 1, 0.5f, 1};

    D3D12Resource* m_resource = nullptr;
};
}  // namespace SE