#pragma once
#include "mikk_tspace/mikktspace.h"
#include "glm/glm.hpp"
#include <vector>

namespace SE
{
class MikkiSpace
{
public:
    struct MikkiSpaceMesh
    {
        std::vector<uint32_t> indices;
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texcoords;
    };

    static bool GetTangents(const MikkiSpaceMesh& mesh, std::vector<glm::vec4>& tangents);

private:
    struct MikkiSpaceContext
    {
        const MikkiSpaceMesh* mesh = nullptr;
        std::vector<glm::vec4> tangets;
    };

    static int GetNumFaces(const SMikkTSpaceContext* context);

    static int GetNumVerticesOfFace(const SMikkTSpaceContext*, const int);

    static void GetPosition(const SMikkTSpaceContext* context, float posOut[3], const int faceIdx, const int vertIdx);

    static void GetNormal(const SMikkTSpaceContext* context, float normOut[3], const int faceIdx, const int vertIdx);

    static void GetTexCoord(const SMikkTSpaceContext* context, float uvOut[2], const int faceIdx, const int vertIdx);

    static void SetTangent(const SMikkTSpaceContext* context,
                           const float tangent[3],
                           const float bitangentSign,
                           const int faceIdx,
                           const int vertIdx);
};
}  // namespace SE