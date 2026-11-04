#pragma once
#include "core/resource.hpp"

namespace tinygltf
{
    class Model;
    struct Primitive;
}  // namespace tinygltf

namespace SE
{
    class Buffer;
    class Model;

    class Mesh : public Resource
    {
    public:
        Mesh(const Model& model, int index, int primitiveIndex);
        ~Mesh();

        static std::string GetPath(const Model& model, int index, int primitiveIndex);

    private:
        friend class Renderer;

        Buffer* m_vertex = nullptr;
        Buffer* m_index = nullptr;
    };
}  // namespace SE