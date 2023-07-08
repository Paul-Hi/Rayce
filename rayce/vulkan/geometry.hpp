/// @file      geometry.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

#include <core/export.hpp>
#include <core/macro.hpp>
#include <core/types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT Geometry
    {
      public:
        Geometry(std::unique_ptr<class Buffer>&& vertexBuffer, uint32 vertexCount, std::unique_ptr<class Buffer>&& indexBuffer, uint32 indexCount);
        ~Geometry();

        const std::unique_ptr<class Buffer>& getVertexBuffer() const
        {
            return pVertexBuffer;
        }

        const std::unique_ptr<class Buffer>& getIndexBuffer() const
        {
            return pIndexBuffer;
        }

        const uint32 getVertexCount() const
        {
            return mVertexCount;
        }

        const uint32 getIndexCount() const
        {
            return mIndexCount;
        }

      private:
        std::unique_ptr<class Buffer> pVertexBuffer;
        std::unique_ptr<class Buffer> pIndexBuffer;

        uint32 mVertexCount;
        uint32 mIndexCount;
    };
} // namespace rayce

#endif // GEOMETRY_HPP