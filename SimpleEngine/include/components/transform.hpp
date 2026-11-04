#pragma once
#include "core/ecs.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>

namespace SE
{
struct Transform
{
    Transform() = default;

    Transform(const glm::vec3& translation, const glm::vec3& scale, const glm::quat& rotation)
        : m_translation(translation), m_scale(scale), m_rotation(rotation)
    {
        SetDirty();
    }

    inline const glm::vec3& GetTranslation() const { return m_translation; }

    inline const glm::vec3& GetScale() const { return m_scale; }

    inline const glm::quat& GetRotation() const { return m_rotation; }

    void SetTranslation(const glm::vec3& position);

    void SetRotation(const glm::quat& rotation);

    void SetScale(const glm::vec3& scale);

    void SetMatrix(const glm::mat4& matrix);

    void SetName(const std::string& newName);

    glm::mat4 GetWorld();

    void SetParent(Entity parent);

    bool HasChildren() const { return m_firstChild != entt::null; }

    bool HasParent() const { return m_parent != entt::null; }

    Entity GetParent() const { return m_parent; }

    std::string name;

    class TransformIterator
    {
    public:
        TransformIterator() = default;
        TransformIterator(Entity ent);
        TransformIterator& operator++();
        bool operator!=(const TransformIterator& iterator);
        Entity operator*();

    private:
        Entity m_current{entt::null};
    };

    TransformIterator begin() { return TransformIterator(m_firstChild); }

    TransformIterator end() { return TransformIterator(); }

private:
    void AddChild(Entity child);

    void RemoveChild(Entity child);

    void SetDirty();

    bool m_dity = false;

    glm::vec3 m_translation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::quat m_rotation = glm::identity<glm::quat>();

    glm::mat4 m_worldMatrix = glm::identity<glm::mat4>();

    // linked list for a scene hierarchi
    Entity m_parent{entt::null};
    Entity m_firstChild{entt::null};
    Entity m_nextSibling{entt::null};
};
}  // namespace SE
