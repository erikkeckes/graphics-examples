#pragma once
#include "rendering/common_dx.hpp"
#include <unordered_map>
#include <vector>

namespace SE
{
class Device;
class D3D12Texture;
class Buffer;
class Pipeline;
class ShaderParamContainer;
class D3D12Resource;

struct RenderingData
{
    bool defaultDepth = true;
    bool defaultTarget = true;
    bool fullscreen = true;
    int top = 0;
    int left = 0;
    int bottom = 0;
    int right = 0;
    std::vector<D3D12Texture*> renderTargets;
    D3D12Texture* depthBuffer = nullptr;
};

class CommandList
{
public:
    DELETEMOVECOPY(CommandList);

    CommandList(Device* device, D3D12_COMMAND_LIST_TYPE type);
    ~CommandList();

    ID3D12GraphicsCommandList* List() const;

    void Reset();

    void Close();

    void StartMarker(const std::string& name);

    void EndMarker();

    void SetIndexBuffer(const Buffer& buffer);

    void SetVertexBuffer(const Buffer& buffer);

    void Present(const D3D12Texture& renderTarget);

    void ClearScreen(const D3D12Texture& renderTarget, int right, int bottom, int left = 0, int top = 0);
    void ClearScreen(const D3D12Texture& renderTarget);

    void ClearDepthStnecil(const D3D12Texture& depthStenil, int right, int bottom, int left = 0, int top = 0);
    void ClearDepthStnecil(const D3D12Texture& depthStenil);

    // sets render target, depth stencil, viewport and rct from device
    void SetMainRenderTarget();

    void SubmitResourceTransitions(uint32_t count, const D3D12_RESOURCE_BARRIER* bariers);

    void SetImGuiRendering();

    void BeginRender(const std::shared_ptr<Pipeline>& pipeline, const RenderingData& data);

    void UavBarrier(const D3D12Texture& texture);

    void CopyTexture(const D3D12Texture& src, const D3D12Texture& dst);

    void CopyTexture(const D3D12Resource& src, const D3D12Texture& dst, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint);

    // only use for buffer that are reused between multiple command lists
    void SetCBV(const Hash& name, const D3D12_GPU_VIRTUAL_ADDRESS& address);

    void SetConstants(const Hash& name, const void* data, size_t size, class UploadBuffer* upload = nullptr);

    void Dispatch(uint32_t x = 1, uint32_t y = 1, uint32_t z = 1);

    void DrawCall(uint32_t indexCount,
                  uint32_t instances = 1,
                  uint32_t indexOffset = 0,
                  uint32_t vertexOffset = 0,
                  uint32_t instanceOffset = 0);

    void SetRectViewportFullScreen();

    void SetRectViewport(int right, int bottom, int left = 0, int top = 0);

    void SetRect(int right, int bottom, int left = 0, int top = 0);

    void SetViewport(int right, int bottom, int left = 0, int top = 0);

private:
    void SetRootConstants(const Hash& name, const uint32_t* values, size_t size);

    void SetPipeline(const std::shared_ptr<Pipeline>& pipeline);

    friend class CommandQueue;

    bool m_cmdListClosed = false;

    const ShaderParamContainer* m_params = nullptr;

    D3D12_COMMAND_LIST_TYPE m_type;
    ID3D12CommandAllocator* m_allocator = nullptr;
    ID3D12GraphicsCommandList* m_list = nullptr;
};
}  // namespace SE