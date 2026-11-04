#pragma once
#include "rendering/common_dx.hpp"

namespace SE
{
class Fence;
class CommandList;

class CommandQueue
{
public:
    DELETEMOVECOPY(CommandQueue);

    D3D12_COMMAND_LIST_TYPE GetType() const;

    uint64_t Execute(CommandList& cmdList);

    void GPUSynchronize(const CommandQueue& queue);

    void Flush() const;

private:
    friend class Device;
    friend class Fence;

    CommandQueue(ID3D12Device2* device, D3D12_COMMAND_LIST_TYPE type);
    ~CommandQueue();

    ID3D12CommandQueue* m_queue;

    Fence* m_fence;

    D3D12_COMMAND_LIST_TYPE m_type;
};
}  // namespace SE