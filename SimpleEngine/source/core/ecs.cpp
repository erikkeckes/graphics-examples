#include "core/ecs.hpp"

using namespace SE;
using namespace std;
using namespace entt;

constexpr float kMaxDeltaTime = 1.0f / 30.0f;

EntityComponentSystem::EntityComponentSystem() = default;

SE::EntityComponentSystem::~EntityComponentSystem() = default;

void EntityComponentSystem::DeleteEntity(Entity e)
{
    assert(Registry.valid(e));
    Registry.emplace_or_replace<Delete>(e);
}

void EntityComponentSystem::UpdateSystems(float dt)
{
    dt = min(dt, kMaxDeltaTime);
    for (auto& s : m_systems) s->Update(dt);
}

void EntityComponentSystem::RenderSystems()
{
    for (auto& s : m_systems) s->Render();
}

void EntityComponentSystem::RemovedDeleted()
{
    bool isDeleteQueueEmpty = false;
    while (!isDeleteQueueEmpty)
    {
        const auto del = Registry.view<Delete>();
        Registry.destroy(del.begin(), del.end());
        isDeleteQueueEmpty = del.empty();
    }
}