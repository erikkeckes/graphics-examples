#include "rendering/core/descriptor_allocators_dx.hpp"
#include "core/engine.hpp"
#include "rendering/core/device_dx.hpp"
#include "rendering/core/resource_dx.hpp"

void SE::DescriptorAllocator::FreeHandle(uint32_t handle)
{
    if (handle != InvalidDescriptor)
    {
        // add only valid handles to free handles
        m_freeHandles.push_back(handle);
    }
}

D3D12_GPU_DESCRIPTOR_HANDLE SE::DescriptorAllocator::GetGPUHandle(uint32_t handle) const
{
    assert(m_heap && "Descriptor allocator heap is invalid");
    assert(m_increment > 0 && "Descriptor allocator inrement is invalid");
    assert(handle != InvalidDescriptor && "Descriptor allocator requesting gpu handle for invalid handle");

    if (!m_shaderVisisble)
    {
        // only shader visible descriptor heaps have valid gpu descriptor handle
        return D3D12_GPU_DESCRIPTOR_HANDLE();
    }

    auto gpuHandle = m_heap->GetGPUDescriptorHandleForHeapStart();
    gpuHandle.ptr += m_increment * handle;
    return gpuHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE SE::DescriptorAllocator::GetCPUHandle(uint32_t handle) const
{
    assert(m_heap && "Descriptor allocator heap is invalid");
    assert(m_increment > 0 && "Descriptor allocator inrement is invalid");
    assert(handle != InvalidDescriptor && "Descriptor allocator requesting gpu handle for invalid handle");
    auto gpuHandle = m_heap->GetCPUDescriptorHandleForHeapStart();
    gpuHandle.ptr += m_increment * handle;
    return gpuHandle;
}

SE::DescriptorAllocator::DescriptorAllocator(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
    : m_count(count)
{
    assert(count > 0 && "Descriptor allocator count must be greater that 0");
    assert(type != D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES && "Descriptor allocator must have valid type");

    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.NumDescriptors = count;
    desc.Type = type;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
    {
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    }

    CheckHR(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap)));

    m_increment = device->GetDescriptorHandleIncrementSize(type);

    // fill free handles
    m_freeHandles.reserve(count);
    for (uint32_t i = 0; i < count; i++)
    {
        m_freeHandles.push_back(i);
    }
}

SE::DescriptorAllocator::~DescriptorAllocator() { COMFREE(m_heap); }

uint32_t SE::DescriptorAllocator::GetFreeHandle()
{
    assert(static_cast<int>(m_freeHandles.size()) > 0 &&
           "Descriptor allocator no free descriptors, increase size or lower used resources");
    auto handle = m_freeHandles.back();
    m_freeHandles.pop_back();
    return handle;
}

SE::CBVSRVUAVDescriptorAllocator::CBVSRVUAVDescriptorAllocator(ID3D12Device2* device, uint32_t count)
    : DescriptorAllocator(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, count)
{
    m_shaderVisisble = true;
}

uint32_t SE::CBVSRVUAVDescriptorAllocator::AllocateCBV(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc)
{
    auto handle = GetFreeHandle();
    Engine.Device().GetDevice()->CreateConstantBufferView(&desc, GetCPUHandle(handle));
    return handle;
}

uint32_t SE::CBVSRVUAVDescriptorAllocator::AllocateSRV(const D3D12_SHADER_RESOURCE_VIEW_DESC& desc,
                                                        const D3D12Resource* resource)
{
    auto handle = GetFreeHandle();
    Engine.Device().GetDevice()->CreateShaderResourceView(resource->GetResource(), &desc, GetCPUHandle(handle));
    return handle;
}

uint32_t SE::CBVSRVUAVDescriptorAllocator::AllocateUAV(const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc,
                                                        const D3D12Resource* resource)
{
    auto handle = GetFreeHandle();
    Engine.Device().GetDevice()->CreateUnorderedAccessView(resource->GetResource(), nullptr, &desc, GetCPUHandle(handle));
    return handle;
}

void SE::CBVSRVUAVDescriptorAllocator::AllocateImgui(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle,
                                                      D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
{
    auto handle = GetFreeHandle();
    *out_cpu_handle = GetCPUHandle(handle);
    *out_gpu_handle = GetGPUHandle(handle);
}

void SE::CBVSRVUAVDescriptorAllocator::FreeImgui(D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE)
{
    uint32_t handle = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE start = m_heap->GetCPUDescriptorHandleForHeapStart();

    // possible that there is more clever solution
    while (handle <= m_count)
    {
        if (start.ptr == cpu_handle.ptr)
        {
            // found linked descriptor
            break;
        }
        // incrememnt to next descriptor
        start.ptr += m_increment;
        handle++;
    }

    assert(handle < m_count && "Failed to find imgui descriptor");
    FreeHandle(handle);
}

uint32_t SE::RTVDescriptorAllocator::AllocateRTV(const D3D12_RENDER_TARGET_VIEW_DESC& desc, const D3D12Resource* resource)
{
    auto handle = GetFreeHandle();
    Engine.Device().GetDevice()->CreateRenderTargetView(resource->GetResource(), &desc, GetCPUHandle(handle));
    return handle;
}

uint32_t SE::RTVDescriptorAllocator::AllocateRTV(const D3D12_RENDER_TARGET_VIEW_DESC& desc,
                                                  const D3D12Resource* resource,
                                                  Device* device)
{
    auto handle = GetFreeHandle();
    device->GetDevice()->CreateRenderTargetView(resource->GetResource(), &desc, GetCPUHandle(handle));
    return handle;
}

SE::RTVDescriptorAllocator::RTVDescriptorAllocator(ID3D12Device2* device, uint32_t count)
    : DescriptorAllocator(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, count)
{
}

uint32_t SE::DSVDescriptorAllocator::AllocateDSV(const D3D12_DEPTH_STENCIL_VIEW_DESC& desc, const D3D12Resource* resource)
{
    auto handle = GetFreeHandle();
    Engine.Device().GetDevice()->CreateDepthStencilView(resource->GetResource(), &desc, GetCPUHandle(handle));
    return handle;
}

SE::DSVDescriptorAllocator::DSVDescriptorAllocator(ID3D12Device2* device, uint32_t count)
    : DescriptorAllocator(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, count)
{
}

uint32_t SE::SamplerDescriptorAllocator::AllocateSample(const D3D12_SAMPLER_DESC& desc)
{
    auto handle = GetFreeHandle();
    Engine.Device().GetDevice()->CreateSampler(&desc, GetCPUHandle(handle));
    return handle;
}

SE::SamplerDescriptorAllocator::SamplerDescriptorAllocator(ID3D12Device2* device, uint32_t count)
    : DescriptorAllocator(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, count)
{
}