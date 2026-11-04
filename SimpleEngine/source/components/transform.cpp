#include "components/transform.hpp"
#include "core/engine.hpp"
#include "tools/log.hpp"
#include <glm/gtx/matrix_decompose.hpp>
#include <entt/entity/helper.hpp>

void SE::Transform::SetTranslation(const glm::vec3& position)
{
    m_translation = position;
    SetDirty();
}

void SE::Transform::SetRotation(const glm::quat& rotation)
{
    m_rotation = rotation;
    SetDirty();
}

void SE::Transform::SetScale(const glm::vec3& scale)
{
    m_scale = scale;
    SetDirty();
}

void SE::Transform::SetMatrix(const glm::mat4& matrix)
{
    m_worldMatrix = matrix;

    glm::vec4 perspective;
    glm::vec3 skew;

    auto result = glm::decompose(matrix, m_scale, m_rotation, m_translation, skew, perspective);
    if (!result)
    {
        Log::Error("Transform failed to decompose matrix");
        assert(false);
    }

    SetDirty();
}

void SE::Transform::SetName(const std::string& newName) { name = newName; }

glm::mat4 SE::Transform::GetWorld()
{
    if (m_dity)
    {
        const auto translation = glm::translate(glm::mat4(1.0f), m_translation);
        const auto rotation = glm::toMat4(m_rotation);
        const auto scale = glm::scale(glm::mat4(1.0f), m_scale);

        if (m_parent == entt::null)
        {
            // no parent can recalculate directly
            m_worldMatrix = translation * rotation * scale;
        }
        else
        {
            assert(Engine.ECS().Registry.valid(m_parent));
            auto& parent = Engine.ECS().Registry.get<Transform>(m_parent);
            m_worldMatrix = parent.GetWorld() * translation * rotation * scale;
        }

        m_dity = false;
    }

    return m_worldMatrix;
}

void SE::Transform::SetParent(Entity parent)
{
    assert(parent == entt::null || Engine.ECS().Registry.valid(parent));

    auto& storage = Engine.ECS().Registry.storage<Transform>();
    Entity thisEntity = entt::to_entity(storage, *this);

    if (m_parent != entt::null && Engine.ECS().Registry.valid(m_parent))
    {
        auto& oldParentTransform = Engine.ECS().Registry.get<Transform>(m_parent);
        oldParentTransform.RemoveChild(thisEntity);
    }

    if (parent != entt::null && Engine.ECS().Registry.valid(parent))
    {
        auto& newParentTransform = Engine.ECS().Registry.get<Transform>(parent);
        newParentTransform.AddChild(thisEntity);
    }

    m_parent = parent;
    SetDirty();
}

void SE::Transform::AddChild(Entity child)
{
    assert(Engine.ECS().Registry.valid(child));

    if (m_firstChild == entt::null)
    {
        m_firstChild = child;
        return;
    }

    auto iterator = m_firstChild;

    while (true)
    {
        auto& transform = Engine.ECS().Registry.get<Transform>(iterator);
        if (transform.m_nextSibling == entt::null)
        {
            transform.m_nextSibling = child;
            break;
        }
        iterator = transform.m_nextSibling;
    }
}

void SE::Transform::RemoveChild(Entity child)
{
    assert(Engine.ECS().Registry.valid(child));

    if (m_firstChild == child)
    {
        auto& transform = Engine.ECS().Registry.get<Transform>(m_firstChild);
        m_firstChild = transform.m_nextSibling;
        return;
    }

    auto iterrator = m_firstChild;
    while (iterrator != entt::null)
    {
        auto& transform = Engine.ECS().Registry.get<Transform>(iterrator);
        if (transform.m_nextSibling == child)
        {
            auto& transform2 = Engine.ECS().Registry.get<Transform>(transform.m_nextSibling);
            transform.m_nextSibling = transform2.m_nextSibling;
            break;
        }
        iterrator = transform.m_nextSibling;
    }
}

void SE::Transform::SetDirty()
{
    m_dity = true;
    for (auto child : *this)
    {
        Engine.ECS().Registry.get<Transform>(child).SetDirty();
    }
}

SE::Transform::TransformIterator::TransformIterator(entt::entity ent) : m_current(ent) {}

SE::Transform::TransformIterator& SE::Transform::TransformIterator::operator++()
{
    assert(Engine.ECS().Registry.valid(m_current));
    auto& transform = Engine.ECS().Registry.get<Transform>(m_current);
    m_current = transform.m_nextSibling;
    return *this;
}

bool SE::Transform::TransformIterator::operator!=(const TransformIterator& iterator)
{
    return m_current != iterator.m_current;
}

SE::Entity SE::Transform::TransformIterator::operator*() { return m_current; }
