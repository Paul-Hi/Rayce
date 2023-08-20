/// @file      geometry.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <vulkan/buffer.hpp>
#include <vulkan/geometry.hpp>

using namespace rayce;

void Geometry::add(std::unique_ptr<Buffer>&& vertexBuffer, uint32 maxVertex, std::unique_ptr<Buffer>&& indexBuffer, uint32 primitiveCount, uint32 materialId, int32 lightId, const std::vector<mat4>& transformationMatrices)
{
    mVertexBuffers.push_back(std::move(vertexBuffer));
    mIndexBuffers.push_back(std::move(indexBuffer));
    mMaxVertices.push_back(maxVertex);
    mPrimitiveCounts.push_back(primitiveCount);
    mMaterialIds.push_back(materialId);
    mLightIds.push_back(lightId);
    mTransformationMatrices.push_back(transformationMatrices);
}
