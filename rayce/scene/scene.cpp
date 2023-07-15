/// @file      scene.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <imgui.h>
#include <scene/scene.hpp>
#include <vulkan/buffer.hpp>
#include <vulkan/commandPool.hpp>
#include <vulkan/device.hpp>
#include <vulkan/geometry.hpp>
#include <vulkan/vertex.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <scene/tiny_obj_loader.h>

using namespace rayce;

Scene::Scene()
    : mMaxVertex(0)
    , mPrimitiveCount(0)
    , mReflectionOpen(true)
{
}

void Scene::loadFromObj(const str& filename, const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<CommandPool>& commandPool)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    str warn;
    str err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str());

    mReflectionInfo.filename = filename;

    std::vector<Vertex> vertices;
    std::vector<uint32> indices;

    for (size_t v = 0; v < attrib.vertices.size(); v += 3)
    {
        Vertex vert;
        vert.position.x() = attrib.vertices[v + 0];
        vert.position.y() = attrib.vertices[v + 1];
        vert.position.z() = attrib.vertices[v + 2];
        vertices.push_back(vert);
    }

    for (auto& shape : shapes)
    {
        mReflectionInfo.shapeNames.push_back(shape.name);
        mReflectionInfo.shapeTriCounts.push_back(shape.mesh.indices.size() / 3);
        for (ptr_size f = 0; f < shape.mesh.indices.size(); f += 3)
        {
            tinyobj::index_t i0 = shape.mesh.indices[f + 0];
            tinyobj::index_t i1 = shape.mesh.indices[f + 1];
            tinyobj::index_t i2 = shape.mesh.indices[f + 2];

            indices.push_back(i0.vertex_index);
            indices.push_back(i1.vertex_index);
            indices.push_back(i2.vertex_index);
        }
    }

    std::unique_ptr<Buffer> vertexBuffer = std::make_unique<Buffer>(logicalDevice, sizeof(Vertex) * vertices.size(),
                                                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                                                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
    std::unique_ptr<Buffer> indexBuffer  = std::make_unique<Buffer>(logicalDevice, sizeof(uint32) * indices.size(),
                                                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                                                       VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);

    vertexBuffer->allocateMemory(logicalDevice, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    indexBuffer->allocateMemory(logicalDevice, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Buffer::uploadDataWithStagingBuffer(logicalDevice, commandPool, *vertexBuffer, vertices);
    Buffer::uploadDataWithStagingBuffer(logicalDevice, commandPool, *indexBuffer, indices);

    mMaxVertex      = static_cast<uint32>(vertices.size() - 1);
    mPrimitiveCount = static_cast<uint32>(indices.size() / 3);

    pGeometry = std::make_unique<Geometry>(std::move(vertexBuffer), vertices.size(), std::move(indexBuffer), indices.size());
}

void Scene::onImGuiRender()
{
    ImGuiWindowFlags window_flags = 0;

    if (!ImGui::Begin("Scene Reflection", &mReflectionOpen, window_flags))
    {
        ImGui::End();
        return;
    }

    ImGui::Spacing();
    ImGui::Text("Scene File: %s", mReflectionInfo.filename.c_str());
    ImGui::Separator();
    ImGui::Separator();
    for (ptr_size i = 0; i < mReflectionInfo.shapeNames.size(); ++i)
    {
        ImGui::Text("Shape: %s with %d triangles", mReflectionInfo.shapeNames[i].c_str(), mReflectionInfo.shapeTriCounts[i]);
        ImGui::Separator();
    }

    ImGui::Separator();
    ImGui::End();
}