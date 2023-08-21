/// @file      geometry.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

namespace rayce
{
    struct RAYCE_API_EXPORT TriangleMeshGeometry
    {
        std::unique_ptr<class Buffer> vertexBuffer;
        std::unique_ptr<class Buffer> indexBuffer;

        uint32 maxVertex;
        uint32 primitiveCount;

        uint32 materialId;
        // int32 lightId;

        std::vector<mat4> transformationMatrices;
    };

    struct RAYCE_API_EXPORT ProceduralSphereGeometry
    {
        std::unique_ptr<class Sphere> sphere;
        std::unique_ptr<class AxisAlignedBoundingBox> boundingBox;

        uint32 materialId;
        int32 lightId;

        std::vector<mat4> transformationMatrices;
    };
    class RAYCE_API_EXPORT Geometry
    {
    public:
        void add(std::unique_ptr<class Buffer>&& vertexBuffer, uint32 maxVertex, std::unique_ptr<class Buffer>&& indexBuffer, uint32 primitiveCount, uint32 materialId, const std::vector<mat4>& transformationMatrices);
        void add(std::unique_ptr<class Sphere>&& sphere, std::unique_ptr<class AxisAlignedBoundingBox>&& boundingBox, uint32 materialId, int32 lightId, const std::vector<mat4>& transformationMatrices);

        const std::vector<TriangleMeshGeometry>& getTriangleMeshes() const
        {
            return mTriangleMeshes;
        }

        const std::vector<ProceduralSphereGeometry>& getProceduralSpheres() const
        {
            return mProceduralSpheres;
        }

    private:
        std::vector<TriangleMeshGeometry> mTriangleMeshes;
        std::vector<ProceduralSphereGeometry> mProceduralSpheres;
    };
} // namespace rayce

#endif // GEOMETRY_HPP
