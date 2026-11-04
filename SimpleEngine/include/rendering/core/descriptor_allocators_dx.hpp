#pragma once
#include "rendering/common_dx.hpp"
#include <vector>

struct ImGui_ImplDX12_InitInfo;

namespace SE
{
class D3D12Resource;
class Device;

class DescriptorAllocator
{
public:
    DELETEMOVECOPY(DescriptorAllocator);

    void FreeHandle(uint32_t handle);

    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t handle) const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t handle) const;

    ID3D12DescriptorHeap* Heap() const { return m_heap; }

protected:
    DescriptorAllocator(ID3D12Device2* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count);
    virtual ~DescriptorAllocator();

    uint32_t GetFreeHandle();

    bool m_shaderVisisble = false;

    uint32_t m_increment = 0;
    uint32_t m_count = 0;
    ID3D12DescriptorHeap* m_heap = nullptr;
    std::vector<uint32_t> m_freeHandles;
};

class CBVSRVUAVDescriptorAllocator : public DescriptorAllocator
{
public:
    uint32_t AllocateCBV(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc);
    uint32_t AllocateSRV(const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, const D3D12Resource* resource);
    uint32_t AllocateUAV(const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc, const D3D12Resource* resource);

    void AllocateImgui(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle);

    void FreeImgui(D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle);

private:
    friend class Device;
    CBVSRVUAVDescriptorAllocator(ID3D12Device2* device, uint32_t count);
};

class RTVDescriptorAllocator : public DescriptorAllocator
{
public:
    uint32_t AllocateRTV(const D3D12_RENDER_TARGET_VIEW_DESC& desc, const D3D12Resource* resource);
    uint32_t AllocateRTV(const D3D12_RENDER_TARGET_VIEW_DESC& desc, const D3D12Resource* resource, Device* device);

private:
    friend class Device;
    RTVDescriptorAllocator(ID3D12Device2* device, uint32_t count);
};

class DSVDescriptorAllocator : public DescriptorAllocator
{
public:
    uint32_t AllocateDSV(const D3D12_DEPTH_STENCIL_VIEW_DESC& desc, const D3D12Resource* resource);

private:
    friend class Device;
    DSVDescriptorAllocator(ID3D12Device2* device, uint32_t count);
};

class SamplerDescriptorAllocator : public DescriptorAllocator
{
public:
    uint32_t AllocateSample(const D3D12_SAMPLER_DESC& desc);

private:
    friend class Device;
    SamplerDescriptorAllocator(ID3D12Device2* device, uint32_t count);
};
}  // namespace SE