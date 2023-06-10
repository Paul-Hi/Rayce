/// @file      geometry.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <vulkan/buffer.hpp>
#include <vulkan/geometry.hpp>

using namespace rayce;

Geometry::Geometry(std::unique_ptr<Buffer>&& vertexBuffer, uint32 vertexCount, std::unique_ptr<Buffer>&& indexBuffer, uint32 indexCount)
    : pVertexBuffer(std::move(vertexBuffer))
    , pIndexBuffer(std::move(indexBuffer))
    , mVertexCount(vertexCount)
    , mIndexCount(indexCount)
{
}

Geometry::~Geometry() {}
