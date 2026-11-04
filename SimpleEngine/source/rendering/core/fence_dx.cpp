#include "rendering/core/fence_dx.hpp"
#include "rendering/core/command_queue_dx.hpp"
#include "core/engine.hpp"
#include "rendering/core/device_dx.hpp"
#include "tools/log.hpp"

SE::Fence::Fence()
{
    Engine.Device().GetDevice()->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    m_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

SE::Fence::Fence(ID3D12Device2* device)
{
    device->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
    m_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

SE::Fence::~Fence()
{
    COMFREE(m_fence);
    CloseHandle(m_event);
}

void SE::Fence::Signal(const CommandQueue& queue)
{
    m_fenceValue++;
    queue.m_queue->Signal(m_fence, m_fenceValue);
}

void SE::Fence::Wait() { WaitFor(m_fenceValue); }

void SE::Fence::WaitFor(uint64_t value)
{
    if (value < GetCurrentValue())
    {
        return;
    }

    m_fence->SetEventOnCompletion(value, m_event);
    WaitForSingleObject(m_event, 0xffffffffUL);
}

uint64_t SE::Fence::GetWaitValue() const { return m_fenceValue; }

uint64_t SE::Fence::GetCurrentValue() const { return m_fence->GetCompletedValue(); }
