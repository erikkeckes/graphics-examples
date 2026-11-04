#include "rendering/miki_space_wrapper.hpp"
#include "tools/log.hpp"

bool SE::MikkiSpace::GetTangents(const MikkiSpaceMesh& mesh, std::vector<glm::vec4>& tangents)
{
    SMikkTSpaceInterface mikkTInterface = {};
    mikkTInterface.m_getNumFaces = GetNumFaces;
    mikkTInterface.m_getNumVerticesOfFace = GetNumVerticesOfFace;
    mikkTInterface.m_getPosition = GetPosition;
    mikkTInterface.m_getNormal = GetNormal;
    mikkTInterface.m_getTexCoord = GetTexCoord;
    mikkTInterface.m_setTSpaceBasic = SetTangent;

    MikkiSpaceContext context = {};
    context.tangets.resize(mesh.positions.size());
    context.mesh = &mesh;

    SMikkTSpaceContext mikkContext = {};
    mikkContext.m_pInterface = &mikkTInterface;
    mikkContext.m_pUserData = &context;

    const bool succes = genTangSpaceDefault(&mikkContext);
    tangents = context.tangets;

    if (!succes)
    {
        SE::Log::Error("Error failed to generate MikktSpaceMesh tangents.");
    }
    else
    {
        SE::Log::Info("Generated tangents using MikktSpace.");
    }

    return succes;
}

int SE::MikkiSpace::GetNumFaces(const SMikkTSpaceContext* context)
{
    auto* c = reinterpret_cast<MikkiSpaceContext*>(context->m_pUserData);
    return static_cast<int>(c->mesh->indices.size() / 3);
}

int SE::MikkiSpace::GetNumVerticesOfFace(const SMikkTSpaceContext*, const int) { return 3; }

void SE::MikkiSpace::GetPosition(const SMikkTSpaceContext* context, float posOut[3], const int faceIdx, const int vertIdx)
{
    auto* c = reinterpret_cast<MikkiSpaceContext*>(context->m_pUserData);
    const uint32_t index = c->mesh->indices[faceIdx * 3 + vertIdx];
    const glm::vec3& position = c->mesh->positions[index];
    posOut[0] = position.x;
    posOut[1] = position.y;
    posOut[2] = position.z;
}

void SE::MikkiSpace::GetNormal(const SMikkTSpaceContext* context, float normOut[3], const int faceIdx, const int vertIdx)
{
    auto* c = reinterpret_cast<MikkiSpaceContext*>(context->m_pUserData);
    const uint32_t index = c->mesh->indices.at(faceIdx * 3 + vertIdx);
    const glm::vec3& normal = c->mesh->normals.at(index);
    normOut[0] = normal.x;
    normOut[1] = normal.y;
    normOut[2] = normal.z;
}

void SE::MikkiSpace::GetTexCoord(const SMikkTSpaceContext* context, float uvOut[2], const int faceIdx, const int vertIdx)
{
    auto* c = reinterpret_cast<MikkiSpaceContext*>(context->m_pUserData);
    const uint32_t index = c->mesh->indices.at(faceIdx * 3 + vertIdx);
    const glm::vec2& uv = c->mesh->texcoords.at(index);
    uvOut[0] = uv.x;
    uvOut[1] = uv.y;
}

void SE::MikkiSpace::SetTangent(const SMikkTSpaceContext* context,
                                 const float tangent[3],
                                 const float bitangentSign,
                                 const int faceIdx,
                                 const int vertIdx)
{
    auto* c = reinterpret_cast<MikkiSpaceContext*>(context->m_pUserData);
    const uint32_t index = c->mesh->indices.at(faceIdx * 3 + vertIdx);
    c->tangets[index] = glm::vec4(tangent[0], tangent[1], tangent[2], bitangentSign);
}
