#include "rendering/core/views_dx.hpp"
#include "rendering/core/descriptor_allocators_dx.hpp"
#include "rendering/core/resource_dx.hpp"
#include "core/engine.hpp"
#include "rendering/core/device_dx.hpp"

void SE::View::SetHandles(const DescriptorAllocator& allocator)
{
    assert(m_handle != InvalidDescriptor && "Getting handles for invalid handle");

    m_cpuHandle = allocator.GetCPUHandle(m_handle);
    m_gpuHandle = allocator.GetGPUHandle(m_handle);
}

SE::VertexView::VertexView(uint32_t size, uint32_t stride, const D3D12Resource* resource)
{
    m_view.BufferLocation = resource->GetGPUAdrress();
    m_view.SizeInBytes = size;
    m_view.StrideInBytes = stride;
}

SE::IndexView::IndexView(uint32_t size, uint32_t stride, const D3D12Resource* resource)
{
    m_view.BufferLocation = resource->GetGPUAdrress();
    m_view.SizeInBytes = size;
    m_view.Format = GetIndexFormat(stride);
}

SE::ConstantBufferView::ConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc)
{
    auto& allocator = Engine.Device().GetCBVSRVUAVAllocator();
    m_handle = allocator.AllocateCBV(desc);
    SetHandles(allocator);
}

SE::ConstantBufferView::~ConstantBufferView() { Engine.Device().GetCBVSRVUAVAllocator().FreeHandle(m_handle); }

SE::ShaderResourceView::ShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, const D3D12Resource* resource)
{
    auto& allocator = Engine.Device().GetCBVSRVUAVAllocator();
    m_handle = allocator.AllocateSRV(desc, resource);
    SetHandles(allocator);
}

SE::ShaderResourceView::~ShaderResourceView() { Engine.Device().GetCBVSRVUAVAllocator().FreeHandle(m_handle); }

SE::UnorderedAccessView::UnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc, const D3D12Resource* resource)
{
    auto& allocator = Engine.Device().GetCBVSRVUAVAllocator();
    m_handle = allocator.AllocateUAV(desc, resource);
    SetHandles(allocator);
}

SE::UnorderedAccessView::~UnorderedAccessView() { Engine.Device().GetCBVSRVUAVAllocator().FreeHandle(m_handle); }

SE::RenderTargetView::RenderTargetView(const D3D12_RENDER_TARGET_VIEW_DESC& desc, const D3D12Resource* resource)
{
    auto& allocator = Engine.Device().GetRTVAllocator();
    m_handle = allocator.AllocateRTV(desc, resource);
    SetHandles(allocator);
}

SE::RenderTargetView::~RenderTargetView() { Engine.Device().GetRTVAllocator().FreeHandle(m_handle); }

SE::DepthStencilView::DepthStencilView(const D3D12_DEPTH_STENCIL_VIEW_DESC& desc, const D3D12Resource* resource)
{
    auto& allocator = Engine.Device().GetDSVAllocator();
    m_handle = allocator.AllocateDSV(desc, resource);
    SetHandles(allocator);
}

SE::DepthStencilView::~DepthStencilView() { Engine.Device().GetDSVAllocator().FreeHandle(m_handle); }

SE::SamplerView::SamplerView(const D3D12_SAMPLER_DESC& desc)
{
    m_handle = Engine.Device().GetSamplerAllocator().AllocateSample(desc);
}

SE::SamplerView::~SamplerView() { Engine.Device().GetSamplerAllocator().FreeHandle(m_handle); }
