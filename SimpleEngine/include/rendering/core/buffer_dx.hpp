#pragma once
#include "rendering/common_dx.hpp"

namespace SE
{

class IndexView;
class VertexView;
class ShaderResourceView;
class ConstantBufferView;
class UnorderedAccessView;
class D3D12Resource;

class Buffer
{
public:
    Buffer() = delete;
    Buffer(const BufferDes& desc);
    ~Buffer();

    DELETEMOVECOPY(Buffer);

    const IndexView& GetIndexView() const;
    const VertexView& GetVertexView() const;
    const ShaderResourceView& GetSRV() const;
    const ConstantBufferView& GetCBV() const;
    const UnorderedAccessView& GetUAV() const;

    uint32_t GetElementCount() const { return m_desc.elementCount; }

private:
    friend class CommandList;

    uint32_t m_size;

    D3D12Resource* m_resource = nullptr;

    IndexView* m_indexView = nullptr;
    VertexView* m_vertexView = nullptr;
    ShaderResourceView* m_srv = nullptr;
    ConstantBufferView* m_cbv = nullptr;
    UnorderedAccessView* m_uav = nullptr;

    BufferDes m_desc;
};
}  // namespace SE