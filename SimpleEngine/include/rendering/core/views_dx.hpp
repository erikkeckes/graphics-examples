#pragma once
#include "rendering/common_dx.hpp"

namespace SE
{
class DescriptorAllocator;
class D3D12Resource;
class Device;

class View
{
public:
    View() = default;
    virtual ~View() = default;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const { return m_cpuHandle; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const { return m_gpuHandle; }

    uint32_t GetHandle() const { return m_handle; }

protected:
    void SetHandles(const DescriptorAllocator& allocator);

    uint32_t m_handle = InvalidDescriptor;

    // may be removed based on what are needed wheres
    D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle{};
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandle{};
};

class VertexView
{
public:
    // needs gpu location
    VertexView(uint32_t size, uint32_t stride, const D3D12Resource* resource);
    ~VertexView() = default;

    const D3D12_VERTEX_BUFFER_VIEW& GetView() const { return m_view; }

private:
    D3D12_VERTEX_BUFFER_VIEW m_view;
};

class IndexView
{
public:
    // needs gpu location
    IndexView(uint32_t size, uint32_t stride, const D3D12Resource* resource);
    ~IndexView() = default;
    const D3D12_INDEX_BUFFER_VIEW& GetView() const { return m_view; }

private:
    D3D12_INDEX_BUFFER_VIEW m_view;
};

class ConstantBufferView : public View
{
public:
    ConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc);
    ~ConstantBufferView();
};

class ShaderResourceView : public View
{
public:
    ShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, const D3D12Resource* resource);
    ~ShaderResourceView();
};

class UnorderedAccessView : public View
{
public:
    UnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc, const D3D12Resource* resource);
    ~UnorderedAccessView();
};

class RenderTargetView : public View
{
public:
    RenderTargetView(const D3D12_RENDER_TARGET_VIEW_DESC& desc, const D3D12Resource* resource);
    ~RenderTargetView();
};

class DepthStencilView : public View
{
public:
    DepthStencilView(const D3D12_DEPTH_STENCIL_VIEW_DESC& desc, const D3D12Resource* resource);
    ~DepthStencilView();
};

class SamplerView : public View
{
public:
    SamplerView(const D3D12_SAMPLER_DESC& desc);
    ~SamplerView();
};
}  // namespace SE