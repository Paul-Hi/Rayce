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
    class RAYCE_API_EXPORT Geometry
    {
    public:
        void add(std::unique_ptr<class Buffer>&& vertexBuffer, uint32 maxVertex, std::unique_ptr<class Buffer>&& indexBuffer, uint32 primitiveCount, uint32 materialId, int32 lightId, const std::vector<mat4>& transformationMatrices);

        const std::vector<std::unique_ptr<class Buffer>>& getVertexBuffers() const
        {
            return mVertexBuffers;
        }

        const std::vector<std::unique_ptr<class Buffer>>& getIndexBuffers() const
        {
            return mIndexBuffers;
        }

        const std::vector<uint32>& getMaxVertices() const
        {
            return mMaxVertices;
        }

        const std::vector<uint32>& getPrimitiveCounts() const
        {
            return mPrimitiveCounts;
        }

        const std::vector<uint32>& getMaterialIds() const
        {
            return mMaterialIds;
        }

        const std::vector<int32>& getLightIds() const
        {
            return mLightIds;
        }

        const std::vector<std::vector<mat4>>& getTransformationMatrices() const
        {
            return mTransformationMatrices;
        }

    private:
        std::vector<std::unique_ptr<class Buffer>> mVertexBuffers;
        std::vector<std::unique_ptr<class Buffer>> mIndexBuffers;

        std::vector<uint32> mMaxVertices;
        std::vector<uint32> mPrimitiveCounts;
        std::vector<uint32> mMaterialIds;
        std::vector<int32> mLightIds;

        std::vector<std::vector<mat4>> mTransformationMatrices;
    };
} // namespace rayce

#endif // GEOMETRY_HPP
