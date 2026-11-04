#include "rendering/core/command_queue_dx.hpp"
#include "rendering/core/fence_dx.hpp"
#include "rendering/core/command_list_dx.hpp"

D3D12_COMMAND_LIST_TYPE SE::CommandQueue::GetType() const { return m_type; }

uint64_t SE::CommandQueue::Execute(CommandList& cmdList)
{
    assert(cmdList.m_type == m_type && "incopatible types between command queue and list");

    cmdList.Close();

    ID3D12CommandList* lists[] = {cmdList.m_list};
    m_queue->ExecuteCommandLists(1, lists);

    m_fence->Signal(*this);

    return m_fence->GetWaitValue();
}

void SE::CommandQueue::GPUSynchronize(const CommandQueue& queue)
{
    m_queue->Wait(queue.m_fence->m_fence, queue.m_fence->GetWaitValue());
}

void SE::CommandQueue::Flush() const
{
    m_fence->Signal(*this);
    m_fence->Wait();
}

SE::CommandQueue::CommandQueue(ID3D12Device2* device, D3D12_COMMAND_LIST_TYPE type) : m_type(type)
{
    D3D12_COMMAND_QUEUE_DESC desc{};
    desc.Type = type;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    CheckHR(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_queue)));

    m_fence = new Fence(device);
}

SE::CommandQueue::~CommandQueue()
{
    delete m_fence;

    COMFREE(m_queue);
}
