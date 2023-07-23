/// @file      geometry.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <vulkan/buffer.hpp>
#include <vulkan/geometry.hpp>

using namespace rayce;

void Geometry::add(std::unique_ptr<Buffer>&& vertexBuffer, uint32 maxVertex, std::unique_ptr<Buffer>&& indexBuffer, uint32 primitiveCount, uint32 materialId, const mat4& transformationMatrix)
{
    mVertexBuffers.push_back(std::move(vertexBuffer));
    mIndexBuffers.push_back(std::move(indexBuffer));
    mMaxVertices.push_back(maxVertex);
    mPrimitiveCounts.push_back(primitiveCount);
    mMaterialIds.push_back(materialId);
    mTransformationMatrices.push_back(transformationMatrix);
}
