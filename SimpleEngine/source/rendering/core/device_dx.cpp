#include "rendering/core/device_dx.hpp"
#include "core/engine.hpp"
#include "rendering/core/command_queue_dx.hpp"
#include "rendering/core/texture_dx.hpp"
#include "rendering/core/descriptor_allocators_dx.hpp"
#include "rendering/core/command_list_dx.hpp"
#include "rendering/core/views_dx.hpp"
#include "rendering/core/fence_dx.hpp"
#include "rendering/core/sampler_dx.hpp"
#include "rendering/core/pipeline_dx.hpp"
#include "rendering/core/pipeline_state_dx.hpp"
#include "rendering/core/root_signature_dx.hpp"
#include "imgui/imgui.h"
#include <dxgidebug.h>

bool SE::Device::ShouldClose()
{
    assert(m_window);
    return glfwWindowShouldClose(m_window) == GLFW_TRUE;
}

void SE::Device::BeginFrame()
{
    if (!m_initialized)
    {
        InitializeSwapChain();
        m_initialized = true;
    }

    // record screen clearing, could change it to using list refference
    auto& list = GetCommandList(QueueType::DirectQueue);
    list.StartMarker("clearing screen");
    list.ClearScreen(*m_renderTargets[m_backbufferIndex]);
    list.ClearDepthStnecil(*m_depthBuffer);
    list.EndMarker();
    m_directQueue->Execute(list);
}

void SE::Device::EndFrame() {}

void SE::Device::Update()
{
    // render imgui to backbuffer
    auto& list = GetCommandList(QueueType::DirectQueue);
    list.StartMarker("transition to present");
    list.Present(*m_renderTargets[m_backbufferIndex]);
    list.EndMarker();
    m_synchValues[m_backbufferIndex] = m_directQueue->Execute(list);

    //// swap buffers
    uint32_t flags = 0x00000200UL;
    uint32_t syncInterval = 0;

    CheckHR(m_swapChain->Present(syncInterval, flags));

    m_backbufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // wait for synch value
    m_directQueue->m_fence->WaitFor(m_synchValues[m_backbufferIndex]);

    glfwPollEvents();

    // reset command list indicies
    m_graphicsIndex = 0;
    m_computeIndex = 0;
    m_copyIndex = 0;

    for (auto* direct : m_graphicsLists[m_backbufferIndex])
    {
        direct->Reset();
    }

    for (auto* copy : m_copysLists[m_backbufferIndex])
    {
        copy->Reset();
    }

    for (auto* compute : m_computeLists[m_backbufferIndex])
    {
        compute->Reset();
    }
}

void SE::Device::Flush()
{
    m_computeQueue->Flush();
    m_copyQueue->Flush();
    m_directQueue->Flush();
}

SE::CommandQueue& SE::Device::GetQueue(QueueType type) const
{
    switch (type)
    {
        case QueueType::DirectQueue:
            return *m_directQueue;
            break;
        case QueueType::ComputeQueue:
            return *m_computeQueue;
            break;
        case QueueType::CopyQueue:
            return *m_copyQueue;
            break;
    }

    return *m_directQueue;
}

SE::CommandList& SE::Device::GetCommandList(QueueType type)
{
    CommandList* list = nullptr;
    switch (type)
    {
        case QueueType::DirectQueue:
        {
            list = GetGraphicsList();
            SetDescriptorHeaps(*list);
        }
        break;
        case QueueType::ComputeQueue:
        {
            list = GetComputeList();
            SetDescriptorHeaps(*list);
        }
        break;
        case QueueType::CopyQueue:
        {
            list = GetCopyList();
        }
        break;
    }

    assert(list && "Failed to get command list");
    return *list;
}

SE::CommandList* SE::Device::GetGraphicsList()
{
    auto& vector = m_graphicsLists[m_backbufferIndex];

    CommandList* list = nullptr;
    if (m_graphicsIndex >= vector.size())
    {
        list = new CommandList(this, D3D12_COMMAND_LIST_TYPE_DIRECT);
        vector.push_back(list);
    }
    else
    {
        list = vector[m_graphicsIndex];
    }

    m_graphicsIndex++;

    return list;
}

SE::CommandList* SE::Device::GetComputeList()
{
    auto& vector = m_computeLists[m_backbufferIndex];
    if (m_computeIndex >= vector.size())
    {
        vector.push_back(new CommandList(this, D3D12_COMMAND_LIST_TYPE_COMPUTE));
    }

    auto* list = vector[m_computeIndex];

    m_computeIndex++;

    return list;
}

SE::CommandList* SE::Device::GetCopyList()
{
    auto& vector = m_copysLists[m_backbufferIndex];
    if (m_copyIndex >= vector.size())
    {
        vector.push_back(new CommandList(this, D3D12_COMMAND_LIST_TYPE_COPY));
    }

    auto* list = vector[m_copyIndex];

    m_copyIndex++;

    return list;
}

SE::CBVSRVUAVDescriptorAllocator& SE::Device::GetCBVSRVUAVAllocator() const
{
    assert(m_CbvSrvUavAllocator && "Invalid allocator");
    return *m_CbvSrvUavAllocator;
}

SE::RTVDescriptorAllocator& SE::Device::GetRTVAllocator() const
{
    assert(m_RtvAllocator && "Invalid allocator");
    return *m_RtvAllocator;
}

SE::DSVDescriptorAllocator& SE::Device::GetDSVAllocator() const
{
    assert(m_DsvAllocator && "Invalid allocator");
    return *m_DsvAllocator;
}

SE::SamplerDescriptorAllocator& SE::Device::GetSamplerAllocator() const
{
    assert(m_SamplerAllocator && "Invalid allocator");
    return *m_SamplerAllocator;
}

const SE::D3D12Texture& SE::Device::GetBackBuffer() const { return *m_renderTargets[m_backbufferIndex]; }

const SE::D3D12Texture& SE::Device::GetDepthBuffer() const { return *m_depthBuffer; }

const D3D12_RECT& SE::Device::GetRect() const { return m_rect; }

const D3D12_VIEWPORT& SE::Device::GetViewPort() const { return m_viewport; }

void SE::Device::SetDescriptorHeaps(CommandList& list) const
{
    ID3D12DescriptorHeap* heaps[] = {m_CbvSrvUavAllocator->m_heap, m_SamplerAllocator->m_heap};
    list.List()->SetDescriptorHeaps(2, heaps);
}

SE::Device::Device()
{
    m_hInstance = GetModuleHandle(nullptr);

    if (!m_hInstance)
    {
        SE::Log::Critical("Window :: Failed to retriece HINSTANCE \n");
        return;
    }

    SE::Log::Info("Using D3D12 Backend");

    for (uint32_t i = 0; i < BufferCount; i++)
    {
        m_renderTargets[i] = nullptr;
        m_synchValues[i] = 0;  // set default synchyng values
    }

    EnableDebugLayer();
    CreateDevice();

    // queue
    m_directQueue = new CommandQueue(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_computeQueue = new CommandQueue(m_device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
    m_copyQueue = new CommandQueue(m_device, D3D12_COMMAND_LIST_TYPE_COPY);

    // descriptor allocators
    m_CbvSrvUavAllocator = new CBVSRVUAVDescriptorAllocator(m_device, CommonDescriptorCount);
    m_RtvAllocator = new RTVDescriptorAllocator(m_device, RTDescriptorCount);
    m_DsvAllocator = new DSVDescriptorAllocator(m_device, DSDescriptorCount);
    m_SamplerAllocator = new SamplerDescriptorAllocator(m_device, SampleDescriptorCount);

    CreateGLFW();
}

SE::Device::~Device()
{
    // flush all work
    Flush();

    // clearing all caches
    D3D12Sampler::m_samplerMap.clear();
    RootSignature::m_signatureCache.clear();
    PipelineStateObject::m_psoCache.clear();
    Pipeline::m_pipelineCache.clear();

    // free render targets
    for (uint32_t i = 0; i < BufferCount; i++)
    {
        delete m_renderTargets[i];
    }

    delete m_depthBuffer;

    // destroying all command lists
    for (uint32_t i = 0; i < BufferCount; i++)
    {
        for (auto* direct : m_graphicsLists[i])
        {
            delete direct;
        }

        for (auto* copy : m_copysLists[i])
        {
            delete copy;
        }

        for (auto* compute : m_computeLists[i])
        {
            delete compute;
        }
    }

    glfwDestroyWindow(m_window);
    glfwTerminate();

    COMFREE(m_swapChain);

    delete m_CbvSrvUavAllocator;
    delete m_RtvAllocator;
    delete m_DsvAllocator;
    delete m_SamplerAllocator;

    delete m_directQueue;
    delete m_computeQueue;
    delete m_copyQueue;
#if CheckReferences
    ID3D12DebugDevice* debugDevice;
    if (SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&debugDevice))))
    {
        debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
        COMFREE(debugDevice);
    }
#endif

    COMFREE(m_device);
    COMFREE(m_factory);
    COMFREE(m_adapter);

#if CheckReferences
    IDXGIDebug1* dxgiDebug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
    {
       dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
       COMFREE(dxgiDebug);
    }
#endif
}

void SE::Device::EnableDebugLayer()
{
#if defined(SE_DEBUG)
    // Enable or disable the debug layer.
    ID3D12Debug6* debugLayers = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayers))))
    {
        SE::Log::Info("Enabled D3D12 debug layer");
        debugLayers->EnableDebugLayer();

#if ENABLE_GPU_VALIDATION_LAYER
        debugLayers->SetEnableGPUBasedValidation(TRUE);
#endif
        // free debug layer
        debugLayers->Release();
    }
#endif
}

void SE::Device::CreateDevice()
{
    UINT factoryFlags = 0;
#if defined(SE_DEBUG)
    factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    CheckHR(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_factory)));

    IDXGIAdapter1* adapter = nullptr;
    IDXGIAdapter1* finalAdapter = nullptr;
    size_t memory = 0;  // memory of final adapter so I can pick adapter with biggest memory
    WCHAR name[128];    // used to store the name
    for (UINT adapterIndex = 0; m_factory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc{};
        CheckHR(adapter->GetDesc1(&desc));

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            COMFREE(adapter);
            // no software gpu adapters are allowed
            continue;
        }

        if (desc.DedicatedVideoMemory > memory)
        {
            memory = desc.DedicatedVideoMemory;
            // description in adapter description is 128 WCHAR characters
            memcpy(name, desc.Description, sizeof(WCHAR) * 128);

            // need to release final adapter before asigning new pointer, ensuring it is released
            COMFREE(finalAdapter);

            finalAdapter = adapter;
        }
        else
        {
            // this adapter was less suitable for application, need to release it
            COMFREE(adapter);
        }
    }

    assert(finalAdapter && "D3D12 failed to find adapter");

    // no release needed for final adapter ownership is transfered to m_adapter
    m_adapter = finalAdapter;

    CheckHR(D3D12CreateDevice(finalAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));

    SE::Log::Info("GPU:  {0}", WStringToString(std::wstring(name)));
    // convert memory to MB from bites
    SE::Log::Info("VRAM: {0} MB", static_cast<int>(memory / (1024 * 1024)));
}

void SE::Device::CreateGLFW()
{
    if (!glfwInit())
    {
        SE::Log::Critical("GLFW init failed");
        assert(false);
    }

    SE::Log::Info("GLFW version {}.{}.{}", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION);

    const uint32_t width = 1280;
    const uint32_t height = 720;

    m_width = width;
    m_height = height;

    m_monitor = glfwGetPrimaryMonitor();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(width, height, "SE - D3D12", nullptr, nullptr);

    if (!m_window)
    {
        glfwTerminate();
        assert(false);
    }

    m_hwnd = glfwGetWin32Window(m_window);

    // set device pointer
    glfwSetWindowUserPointer(m_window, this);
    // resize callback using lambda
    glfwSetWindowSizeCallback(m_window, [](GLFWwindow*, int width, int height) { SE::Engine.Device().Resize(width, height); });
}

void SE::Device::InitializeSwapChain()
{
    m_swapchainDesc = {};

    m_swapchainDesc.Width = m_width;
    m_swapchainDesc.Height = m_height;

    m_swapchainDesc.Format = BackBufferFormat;
    m_swapchainDesc.Stereo = FALSE;
    m_swapchainDesc.SampleDesc = {1, 0};
    m_swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    m_swapchainDesc.BufferCount = BufferCount;
    m_swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
    m_swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    m_swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    m_swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    CreateSwapchain();
}

void SE::Device::CreateSwapchain()
{
    // add flush of all work first
    Flush();

    // destroy old swapchain
    COMFREE(m_swapChain);

    // free render targets
    for (uint32_t i = 0; i < BufferCount; i++)
    {
        delete m_renderTargets[i];
    }

    delete m_depthBuffer;

    // temporaly comptr to lower level swapchain
    IDXGISwapChain1* temp = nullptr;
    CheckHR(m_factory->CreateSwapChainForHwnd(m_directQueue->m_queue, m_hwnd, &m_swapchainDesc, nullptr, nullptr, &temp));

    // query higher version of interface
    CheckHR(temp->QueryInterface(&m_swapChain));

    // ssets srgb space for swapchain
    // m_swapChain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);

    // release temporaly object
    COMFREE(temp);

    TextureDesc desc;
    desc.InitAsRenderTarget(m_swapchainDesc.Width, m_swapchainDesc.Height, BackBufferFormat);
    desc.clearColor[0] = 0.25f;
    desc.clearColor[1] = 0.25f;
    desc.clearColor[2] = 0.25f;
    desc.clearColor[3] = 1;

    // create new render targets
    for (uint32_t i = 0; i < BufferCount; i++)
    {
        m_renderTargets[i] = new D3D12Texture(desc, m_swapChain, i);
    }

    TextureDesc depth{};
    depth.InitAsDepthStencil(m_width, m_height, DeptBufferFormat).AddFlags(TextureDesc::SRV).SetMips(1);

    m_depthBuffer = new D3D12Texture(depth);

    m_viewport = CD3DX12_VIEWPORT(0.f, 0.f, static_cast<FLOAT>(m_width), static_cast<FLOAT>(m_height));
    m_rect.left = 0;
    m_rect.top = 0;
    m_rect.bottom = m_height;
    m_rect.right = m_width;

    m_backbufferIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void SE::Device::Resize(int width, int height)
{
    m_width = std::max(1, width);
    m_height = std::max(1, height);

    // recreate swapchain
    m_swapchainDesc.Width = m_width;
    m_swapchainDesc.Height = m_height;

    CreateSwapchain();
}