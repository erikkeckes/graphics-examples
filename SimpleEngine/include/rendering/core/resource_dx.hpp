#pragma once
#include "rendering/common_dx.hpp"
#include <vector>

namespace SE
{
class CommandList;

class D3D12Resource
{
public:
    DELETEMOVECOPY(D3D12Resource);

    // resource for buffer
    D3D12Resource(const BufferDes& desc);
    // resource for a texture
    D3D12Resource(const TextureDesc& desc);
    // resource for swapchain
    D3D12Resource(const TextureDesc& desc, ID3D12Resource* resource);
    ~D3D12Resource();

    // add later when I have a command list
    void Transition(CommandList& list,
                    D3D12_RESOURCE_STATES state,
                    uint32_t subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

    void UavBarier(CommandList& list);

    ID3D12Resource* GetResource() const;

    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAdrress() const;

    D3D12Resource* CreateUploadResource() const;

    const D3D12_RESOURCE_DESC& GetDesc() const { return m_desc; }

    void Write(const void* data, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint);

private:
    D3D12Resource() = default;
    void SetState(D3D12_RESOURCE_STATES state);

    D3D12_CLEAR_VALUE m_optimizedColor{};
    D3D12_RESOURCE_DESC m_desc{};
    ID3D12Resource* m_resource = nullptr;

    std::vector<D3D12_RESOURCE_STATES> m_states;
};
}  // namespace SE