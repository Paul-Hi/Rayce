/// @file      vertex.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef VERTEX_HPP
#define VERTEX_HPP

namespace rayce
{
    struct RAYCE_API_EXPORT Vertex
    {
    public:
        vec3 position;
        vec2 uv;

        static VkVertexInputBindingDescription getVertexInputBindingDescription()
        {
            VkVertexInputBindingDescription vertexInputBindingDescription{};
            vertexInputBindingDescription.binding   = 0;
            vertexInputBindingDescription.stride    = sizeof(Vertex);
            vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return vertexInputBindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 2> getVertexInputAttributeDescription()
        {
            std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributeDescription{};
            vertexInputAttributeDescription[0].binding  = 0;
            vertexInputAttributeDescription[0].location = 0;
            vertexInputAttributeDescription[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
            vertexInputAttributeDescription[0].offset   = offsetof(Vertex, position);

            vertexInputAttributeDescription[1].binding  = 0;
            vertexInputAttributeDescription[1].location = 1;
            vertexInputAttributeDescription[1].format   = VK_FORMAT_R32G32_SFLOAT;
            vertexInputAttributeDescription[1].offset   = offsetof(Vertex, uv);

            return vertexInputAttributeDescription;
        }

        static ptr_size getSize()
        {
            return sizeof(Vertex);
        }
    };
} // namespace rayce

#endif // VERTEX_HPP
