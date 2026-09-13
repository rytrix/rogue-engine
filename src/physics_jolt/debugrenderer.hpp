#pragma once

#include <Jolt/Renderer/DebugRendererSimple.h>

class GlobalAppData;

namespace Physics {

class DebugRenderer : public JPH::DebugRendererSimple, public NoCopyNoMove {
public:
    DebugRenderer(GlobalAppData* app_data);
    ~DebugRenderer() override = default;

    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
    void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow = ECastShadow::Off) override;

    void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor = JPH::Color::sWhite, float inHeight = 0.5f) override;
    // Batch CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) override;
    // Batch CreateTriangleBatch(const Vertex* inVertices, int inVertexCount, const u32* inIndices, int inIndexCount) override;
    // void DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef& inGeometry, ECullMode inCullMode = ECullMode::CullBackFace, ECastShadow inCastShadow = ECastShadow::Off, EDrawMode inDrawMode = EDrawMode::Wireframe) override;

private:
    GlobalAppData* m_app_data = nullptr;
};

} // namespace Physics
