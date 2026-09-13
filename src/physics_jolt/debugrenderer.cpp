#include "debugrenderer.hpp"

#include "helpers.hpp"

#include "../app_data.hpp"

namespace Physics {

DebugRenderer::DebugRenderer(GlobalAppData* app_data)
    : m_app_data(app_data)
{
    Initialize();
}

void DebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
    m_app_data->m_line_renderer.add_line(Physics::vec3_to_vec3(inFrom), Physics::vec3_to_vec3(inTo), inColor.GetUInt32());
}

void DebugRenderer::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, [[maybe_unused]] ECastShadow inCastShadow)
{
    m_app_data->m_line_renderer.add_triangle(Physics::vec3_to_vec3(inV1), Physics::vec3_to_vec3(inV2), Physics::vec3_to_vec3(inV3), inColor.GetUInt32());
}

void DebugRenderer::DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight)
{
}

// DebugRenderer::Batch DebugRenderer::CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount)
// {
//     Batch test;
//     return test;
// }
//
// DebugRenderer::Batch DebugRenderer::CreateTriangleBatch(const DebugRenderer::Vertex* inVertices, int inVertexCount, const u32* inIndices, int inIndexCount)
// {
//     Batch test;
//     return test;
// }
//
// void DebugRenderer::DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef& inGeometry, ECullMode inCullMode, ECastShadow inCastShadow, EDrawMode inDrawMode)
// {
// }

} // namespace Physics
