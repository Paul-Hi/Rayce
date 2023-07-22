/// @file      scene.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <filesystem>
#include <imgui.h>
#include <scene/rayceScene.hpp>
#include <vulkan/buffer.hpp>
#include <vulkan/commandPool.hpp>
#include <vulkan/device.hpp>
#include <vulkan/geometry.hpp>
#include <vulkan/image.hpp>
#include <vulkan/imageView.hpp>
#include <vulkan/vertex.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <scene/tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <scene/stb_image.h>

using namespace rayce;
namespace fs = std::filesystem;

RayceScene::RayceScene()
    : mMaxVertex(0)
    , mPrimitiveCount(0)
    , mReflectionOpen(true)
{
}

void RayceScene::loadFromObj(const str& filename, const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<CommandPool>& commandPool)
{
    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(filename))
    {
        RAYCE_LOG_ERROR("Can not load: %s", filename.c_str());
        if (!reader.Error().empty())
        {
            RAYCE_LOG_ERROR("Tinyobj error: %s", reader.Error().c_str());
        }
        return;
    }

    if (!reader.Warning().empty())
    {
        RAYCE_LOG_ERROR("Tinyobj warning: %s", reader.Warning().c_str());
    }

    RAYCE_LOG_INFO("Loaded: %s", filename.c_str());

    auto& attrib    = reader.GetAttrib();
    auto& shapes    = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    mReflectionInfo.filename = filename;

    // geometry

    std::vector<Vertex> vertices;
    std::vector<uint32> indices;

    for (ptr_size v = 0; v < attrib.vertices.size(); v += 3)
    {
        Vertex vert;
        vert.position.x() = attrib.vertices[static_cast<uint32>(v) + 0];
        vert.position.y() = attrib.vertices[static_cast<uint32>(v) + 1];
        vert.position.z() = attrib.vertices[static_cast<uint32>(v) + 2];
        vertices.push_back(vert);
    }

    for (auto& shape : shapes)
    {
        mReflectionInfo.shapeNames.push_back(shape.name);
        mReflectionInfo.shapeTriCounts.push_back(static_cast<uint32>(shape.mesh.indices.size() / 3));

        // FIXME: shape.mesh.material_ids

        for (ptr_size f = 0; f < shape.mesh.indices.size(); f += 3)
        {
            tinyobj::index_t i0 = shape.mesh.indices[static_cast<uint32>(f) + 0];
            tinyobj::index_t i1 = shape.mesh.indices[static_cast<uint32>(f) + 1];
            tinyobj::index_t i2 = shape.mesh.indices[static_cast<uint32>(f) + 2];

            Vertex& vert0 = vertices[static_cast<uint32>(i0.vertex_index)];
            Vertex& vert1 = vertices[static_cast<uint32>(i1.vertex_index)];
            Vertex& vert2 = vertices[static_cast<uint32>(i2.vertex_index)];

            if (attrib.texcoords.empty() || (i0.texcoord_index < 0) || (i1.texcoord_index < 0) || (i2.texcoord_index < 0))
            {
                vert0.uv[0] = 0.0f;
                vert0.uv[1] = 0.0f;
                vert1.uv[0] = 0.0f;
                vert1.uv[1] = 0.0f;
                vert2.uv[0] = 0.0f;
                vert2.uv[1] = 0.0f;
            }
            else
            {
                // flip y coord.
                vert0.uv[0] = attrib.texcoords[2 * i0.texcoord_index];
                vert0.uv[1] = 1.0f - attrib.texcoords[2 * i0.texcoord_index + 1];
                vert1.uv[0] = attrib.texcoords[2 * i1.texcoord_index];
                vert1.uv[1] = 1.0f - attrib.texcoords[2 * i1.texcoord_index + 1];
                vert2.uv[0] = attrib.texcoords[2 * i2.texcoord_index];
                vert2.uv[1] = 1.0f - attrib.texcoords[2 * i2.texcoord_index + 1];
            }

            indices.push_back(static_cast<uint32>(i0.vertex_index));
            indices.push_back(static_cast<uint32>(i1.vertex_index));
            indices.push_back(static_cast<uint32>(i2.vertex_index));
        }
    }

    std::unique_ptr<Buffer> vertexBuffer = std::make_unique<Buffer>(logicalDevice, sizeof(Vertex) * vertices.size(),
                                                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                                                        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    std::unique_ptr<Buffer> indexBuffer  = std::make_unique<Buffer>(logicalDevice, sizeof(uint32) * indices.size(),
                                                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                                                       VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    vertexBuffer->allocateMemory(logicalDevice, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    indexBuffer->allocateMemory(logicalDevice, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    Buffer::uploadDataWithStagingBuffer(logicalDevice, commandPool, *vertexBuffer, vertices);
    Buffer::uploadDataWithStagingBuffer(logicalDevice, commandPool, *indexBuffer, indices);

    mMaxVertex      = static_cast<uint32>(vertices.size() - 1);
    mPrimitiveCount = static_cast<uint32>(indices.size() / 3);

    pGeometry = std::make_unique<Geometry>(std::move(vertexBuffer), vertices.size(), std::move(indexBuffer), indices.size());

    // textures
    RAYCE_LOG_INFO("Loading %d materials", static_cast<uint32>(materials.size()));
    for (auto& material : materials)
    {
        if (material.diffuse_texname.length() > 0 && mImageCache.find(material.diffuse_texname) == mImageCache.end())
        {
            int32 w, h, components;

            str texture_filename = material.diffuse_texname;
            if (!fs::exists(texture_filename))
            {
                texture_filename = fs::path(filename).parent_path().concat("\\" + texture_filename).string();
                if (!fs::exists(texture_filename))
                {
                    RAYCE_ABORT("Can not find %s nor %s", material.diffuse_texname.c_str(), texture_filename.c_str());
                }
            }

            mImageCache[material.diffuse_texname] =
                stbi_load(texture_filename.c_str(), &w, &h, &components, STBI_rgb_alpha);
            if (!mImageCache[material.diffuse_texname])
            {
                RAYCE_LOG_ERROR("Can not load: %s", texture_filename.c_str());
            }
            RAYCE_LOG_INFO("Loaded: %s", texture_filename.c_str());

            uint32 imageSize = w * h * 4;

            uint32 width  = static_cast<uint32>(w);
            uint32 height = static_cast<uint32>(h);
            VkExtent2D extent{ width, height };
            mImages.push_back(std::make_unique<Image>(logicalDevice, extent, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
            auto& addedImage = mImages.back();
            addedImage->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            VkExtent3D extent3D{ width, height, 1 };
            Image::uploadImageDataWithStagingBuffer(logicalDevice, commandPool, *addedImage, mImageCache[material.diffuse_texname], imageSize, extent3D);
            addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            mImageViews.push_back(std::make_unique<ImageView>(logicalDevice, *addedImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT));
        }
    }
}

void RayceScene::onImGuiRender()
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
        if (i < mReflectionInfo.shapeNames.size() - 1)
        {
            ImGui::Separator();
        }
    }

    ImGui::End();
}
