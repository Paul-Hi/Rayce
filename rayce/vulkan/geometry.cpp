/// @file      geometry.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <hostDeviceInterop.slang>
#include <vulkan/buffer.hpp>
#include <vulkan/geometry.hpp>

using namespace rayce;

void Geometry::add(std::unique_ptr<Buffer>&& vertexBuffer, uint32 maxVertex, std::unique_ptr<Buffer>&& indexBuffer, uint32 primitiveCount, uint32 materialId, int32 lightId, const std::vector<mat4>& transformationMatrices)
{
    TriangleMeshGeometry geom;
    geom.vertexBuffer   = std::move(vertexBuffer);
    geom.indexBuffer    = std::move(indexBuffer);
    geom.maxVertex      = maxVertex;
    geom.primitiveCount = primitiveCount;
    geom.materialId     = materialId;
    geom.lightId = lightId;
    geom.transformationMatrices.insert(geom.transformationMatrices.end(), transformationMatrices.begin(), transformationMatrices.end());

    mTriangleMeshes.push_back(std::move(geom));
}

void Geometry::add(std::unique_ptr<Sphere>&& sphere, std::unique_ptr<AxisAlignedBoundingBox>&& boundingBox, uint32 materialId, int32 lightId, const std::vector<mat4>& transformationMatrices)
{
    ProceduralSphereGeometry geom;
    geom.sphere = std::move(sphere);
    geom.boundingBox = std::move(boundingBox);
    geom.materialId = materialId;
    geom.lightId = lightId;
    geom.transformationMatrices.insert(geom.transformationMatrices.end(), transformationMatrices.begin(), transformationMatrices.end());

    mProceduralSpheres.push_back(std::move(geom));
}
