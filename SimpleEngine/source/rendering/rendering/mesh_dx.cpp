#include "resources/mesh.hpp"
#include "resources/model.hpp"
#include "glm/glm.hpp"
#include "rendering/core/buffer_dx.hpp"
#include "rendering/miki_space_wrapper.hpp"

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 tangent;
};

template <typename T>
std::vector<T> LoadAttribute(const std::string& name, const tinygltf::Model& model, const tinygltf::Primitive& primitive)
{
    std::vector<T> vec;

    const auto attribute = primitive.attributes.find(name);
    if (attribute == primitive.attributes.end())
    {
        return vec;
    }

    const tinygltf::Accessor& accessor = model.accessors[attribute->second];
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
    const void* data = &buffer.data[bufferView.byteOffset + accessor.byteOffset];

    vec.assign(reinterpret_cast<const T*>(data), reinterpret_cast<const T*>(data) + accessor.count);
    return vec;
}

std::vector<uint32_t> LoadIndicies(const tinygltf::Model& model, const tinygltf::Primitive& primitive)
{
    std::vector<uint32_t> vec;

    const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
    const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
    const void* data = &buffer.data[bufferView.byteOffset + accessor.byteOffset];

    vec.resize(accessor.count);
    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
    {
        memcpy(vec.data(), data, sizeof(uint32_t) * accessor.count);
    }
    else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
    {
        const auto* castedData = reinterpret_cast<const uint16_t*>(data);
        for (auto i = 0; i < static_cast<int>(accessor.count); i++)
        {
            vec[i] = static_cast<uint32_t>(castedData[i]);
        }
    }

    return vec;
}

SE::Mesh::Mesh(const Model& model, int index, int primitiveIndex) : Resource(ResourceType::Mesh)
{
    const auto& doc = model.GetModel();
    assert(index < static_cast<int>(doc.meshes.size()) && "Loading mesh outside of bounds");
    assert(primitiveIndex < static_cast<int>(doc.meshes[index].primitives.size()) && "Loading mesh primitve outside of bounds");

    // right now supports only one primitive
    const auto& primitive = doc.meshes[index].primitives[primitiveIndex];

    std::vector<uint32_t> indicies = LoadIndicies(doc, primitive);
    std::vector<glm::vec3> positions = LoadAttribute<glm::vec3>("POSITION", doc, primitive);
    std::vector<glm::vec2> uvs = LoadAttribute<glm::vec2>("TEXCOORD_0", doc, primitive);
    std::vector<glm::vec3> normals = LoadAttribute<glm::vec3>("NORMAL", doc, primitive);
    std::vector<glm::vec4> tangents = LoadAttribute<glm::vec4>("TANGENT", doc, primitive);

    if (tangents.empty())
    {
        MikkiSpace::MikkiSpaceMesh mikkiMesh;
        mikkiMesh.indices = indicies;
        mikkiMesh.normals = normals;
        mikkiMesh.positions = positions;
        mikkiMesh.texcoords = uvs;

        MikkiSpace::GetTangents(mikkiMesh, tangents);
    }

    auto verticies = std::vector<Vertex>(positions.size());
    for (auto i = 0; i < static_cast<int>(positions.size()); i++)
    {
        verticies[i] = {positions[i], normals[i], uvs[i], tangents[i]};
    }

    BufferDes vertexDesc{};
    vertexDesc.InitAsVertexBuffer(sizeof(Vertex), static_cast<uint32_t>(positions.size()), verticies.data());
    m_vertex = new Buffer(vertexDesc);

    BufferDes indexDesc{};
    indexDesc.InitAsIndexBuffer(sizeof(uint32_t), static_cast<uint32_t>(indicies.size()), indicies.data());
    m_index = new Buffer(indexDesc);
}

SE::Mesh::~Mesh()
{
    delete m_vertex;
    delete m_index;
}

std::string SE::Mesh::GetPath(const Model& model, int index, int primitiveIndex)
{
    const auto& doc = model.GetModel();
    return model.GetPath() + " | " + doc.meshes[index].name + " | " + std::to_string(primitiveIndex);
}
