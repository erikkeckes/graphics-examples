#pragma once
// needs handle for the window
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"
#include "rendering/common_dx.hpp"
#include <array>
#include <vector>

namespace SE
{
constexpr uint32_t BufferCount = 2;
constexpr uint32_t CommonDescriptorCount = 4096;
constexpr uint32_t RTDescriptorCount = 64;
constexpr uint32_t DSDescriptorCount = 64;
constexpr uint32_t SampleDescriptorCount = 64;

class CommandQueue;
class CommandList;
class D3D12Texture;
class CBVSRVUAVDescriptorAllocator;
class RTVDescriptorAllocator;
class DSVDescriptorAllocator;
class SamplerDescriptorAllocator;

class Device
{
public:
    bool ShouldClose();

    void BeginFrame();

    void EndFrame();
    void Update();

    int GetWidth() { return m_width; }
    int GetHeight() { return m_height; }

    float GetMonitorUIScale() const { return 1.f; }

    void Flush();

    CommandQueue& GetQueue(QueueType type) const;

    CommandList& GetCommandList(QueueType type);

    GLFWwindow* GetWindow() const { return m_window; }

    ID3D12Device2* GetDevice() const { return m_device; }

    CBVSRVUAVDescriptorAllocator& GetCBVSRVUAVAllocator() const;
    RTVDescriptorAllocator& GetRTVAllocator() const;
    DSVDescriptorAllocator& GetDSVAllocator() const;
    SamplerDescriptorAllocator& GetSamplerAllocator() const;

    const D3D12Texture& GetBackBuffer() const;

    const D3D12Texture& GetDepthBuffer() const;

    const D3D12_RECT& GetRect() const;

    const D3D12_VIEWPORT& GetViewPort() const;

    void SetDescriptorHeaps(CommandList& list) const;

private:
    friend class EngineClass;
    Device();
    ~Device();

    static void EnableDebugLayer();

    void CreateDevice();

    void CreateGLFW();

    void InitializeSwapChain();

    void CreateSwapchain();

    void Resize(int width, int height);

    CommandList* GetGraphicsList();

    CommandList* GetComputeList();

    CommandList* GetCopyList();

    int m_width = -1;
    int m_height = -1;

    bool m_initialized = false;

    HINSTANCE m_hInstance;
    HWND m_hwnd;

    DXGI_SWAP_CHAIN_DESC1 m_swapchainDesc;

    D3D12_RECT m_rect;
    D3D12_VIEWPORT m_viewport;

    uint32_t m_backbufferIndex;

    GLFWwindow* m_window = nullptr;
    GLFWmonitor* m_monitor = nullptr;

    IDXGIFactory7* m_factory = nullptr;
    IDXGIAdapter1* m_adapter = nullptr;
    ID3D12Device2* m_device = nullptr;

    IDXGISwapChain4* m_swapChain = nullptr;

    // queues
    CommandQueue* m_directQueue = nullptr;
    CommandQueue* m_computeQueue = nullptr;
    CommandQueue* m_copyQueue = nullptr;

    // descriptor allocators
    CBVSRVUAVDescriptorAllocator* m_CbvSrvUavAllocator = nullptr;
    RTVDescriptorAllocator* m_RtvAllocator = nullptr;
    DSVDescriptorAllocator* m_DsvAllocator = nullptr;
    SamplerDescriptorAllocator* m_SamplerAllocator = nullptr;

    std::array<D3D12Texture*, BufferCount> m_renderTargets;

    D3D12Texture* m_depthBuffer = nullptr;

    std::array<uint64_t, BufferCount> m_synchValues;

    std::array<std::vector<CommandList*>, BufferCount> m_graphicsLists;
    std::array<std::vector<CommandList*>, BufferCount> m_computeLists;
    std::array<std::vector<CommandList*>, BufferCount> m_copysLists;

    uint32_t m_graphicsIndex = 0;
    uint32_t m_computeIndex = 0;
    uint32_t m_copyIndex = 0;
};
}  // namespace SE