#include "resources/model.hpp"
#include "core/engine.hpp"
#include "components/transform.hpp"
#include "tools/log.hpp"
#include "core/resources.hpp"
#include "resources/mesh.hpp"
#include "resources/image.hpp"
#include "components/rendering_components.hpp"
#include "tools/tools.hpp"
#include <glm/gtc/type_ptr.hpp>

std::vector<int> DetectSrgbTextures(const tinygltf::Model& model)
{
    std::vector<int> srgbTextures;
    for (const auto& mat : model.materials)
    {
        if (mat.pbrMetallicRoughness.baseColorTexture.index >= 0)
        {
            const auto& text = model.textures[mat.pbrMetallicRoughness.baseColorTexture.index];
            if (text.source >= 0)
            {
                srgbTextures.push_back(text.source);
            }
        }
        if (mat.emissiveTexture.index >= 0)
        {
            const auto& text = model.textures[mat.emissiveTexture.index];
            if (text.source >= 0)
            {
                srgbTextures.push_back(text.source);
            }
        }
    }

    return srgbTextures;
}

SE::Model::Model(FileIO::Directory directory, const std::string& path) : Resource(ResourceType::Model)
{
    auto modelPath = Engine.FileIO().GetPath(directory, path);
    m_path = modelPath;

    tinygltf::TinyGLTF loader;
    std::string errors;
    std::string warnings;
    bool success = false;

    success = loader.LoadASCIIFromFile(&m_model, &errors, &warnings, modelPath);

    if (!errors.empty())
    {
        Log::Error("Model loading error {}", errors);
    }

    if (!warnings.empty())
    {
        Log::Warn("Model loading warnings {}", warnings);
    }

    if (!success)
    {
        Log::Error("Failed to load model path {}", modelPath);
        return;
    }

    for (auto i = 0u; i < m_model.meshes.size(); i++)
    {
        const auto& mesh = m_model.meshes[i];
        std::vector<std::shared_ptr<Mesh>> primitives;
        for (auto j = 0u; j < mesh.primitives.size(); j++)
        {
            primitives.push_back(Engine.Resources().Load<Mesh>(*this, i, j));
        }
        m_meshes.push_back(primitives);
    }

    auto srgbTexture = DetectSrgbTextures(m_model);

    for (auto i = 0u; i < m_model.images.size(); i++)
    {
        bool srgb = false;
        for (auto index : srgbTexture)
        {
            if (static_cast<uint64_t>(index) == i)
            {
                srgb = true;
                break;
            }
        }

        m_images.push_back(Engine.Resources().Load<Image>(*this, i, srgb));
    }

    for (auto i = 0u; i < m_model.samplers.size(); i++)
    {
        m_samplers.push_back(std::make_shared<Sampler>(*this, i));
    }

    for (auto i = 0u; i < m_model.lights.size(); i++)
    {
        m_lights.push_back(std::make_shared<Light>(*this, i));
    }

    for (auto i = 0u; i < m_model.textures.size(); i++)
    {
        m_textures.push_back(std::make_shared<Texture>(*this, i));
    }

    for (auto i = 0u; i < m_model.materials.size(); i++)
    {
        m_materials.push_back(std::make_shared<Material>(*this, i));
    }
}

void SE::Model::InstanciateModel(Entity entity, bool lights) const
{
    const auto& scene = m_model.scenes[0];

    for (const auto& node : scene.nodes)
    {
        InstaciateNode(node, entity, lights);
    }
}

void SE::Model::InstaciateNode(const uint32_t nodeIndex, Entity parent, bool lights) const
{
    const auto& node = m_model.nodes[nodeIndex];
    const auto entity = Engine.ECS().CreateEntity();

    Transform transform{};
    transform.SetName(node.name);

    // check for entire matrix
    if (!node.matrix.empty())
    {
        transform.SetMatrix(glm::make_mat4(node.matrix.data()));
    }
    else
    {
        if (!node.translation.empty())
        {
            transform.SetTranslation(to_vec3(node.translation));
        }
        if (!node.rotation.empty())
        {
            transform.SetRotation(to_quat(node.rotation));
        }
        if (!node.scale.empty())
        {
            transform.SetTranslation(to_vec3(node.scale));
        }
    }
    auto& firstTransform = Engine.ECS().CreateComponent<Transform>(entity, transform);
    firstTransform.SetParent(parent);

    // check mesh node
    if (node.mesh >= 0)
    {
        const auto& mesh = m_model.meshes[node.mesh];
        const auto& primitives = m_meshes[node.mesh];

        firstTransform.SetName(node.name + "0");
        Engine.ECS().CreateComponent<MeshInstance>(entity, primitives[0], GetPrimitiveMaterial(mesh.primitives[0]));

        for (auto i = 1u; i < primitives.size(); i++)
        {
            const auto entity2 = Engine.ECS().CreateEntity();
            // Transform
            auto& transform2 = Engine.ECS().CreateComponent<Transform>(entity2, transform);
            transform2.SetParent(parent);
            transform2.SetName(node.name + std::to_string(i));

            Engine.ECS().CreateComponent<MeshInstance>(entity2, primitives[i], GetPrimitiveMaterial(mesh.primitives[i]));
        }
    }

    if (lights)
    {
        if (node.extensions.find("KHR_lights_punctual") != node.extensions.end())
        {
            const auto& lightExtension = node.extensions.at("KHR_lights_punctual");
            const auto& light = lightExtension.Get("light");
            int i = light.GetNumberAsInt();
            Engine.ECS().CreateComponent<Light>(entity, *m_lights[i]);
        }
    }

    for (auto child : node.children)
    {
        InstaciateNode(child, entity, lights);
    }
}

std::shared_ptr<SE::Material> SE::Model::GetPrimitiveMaterial(const tinygltf::Primitive& primitive) const
{
    if (primitive.material != -1)
    {
        return m_materials[primitive.material];
    }
    return std::make_shared<Material>();
}