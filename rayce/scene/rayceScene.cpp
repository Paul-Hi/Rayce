/// @file      scene.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <core/material.hpp>
#include <filesystem>
#include <functional>
#include <imgui.h>
#include <scene/gltfHelper.hpp>
#include <scene/rayceScene.hpp>
#include <vulkan/buffer.hpp>
#include <vulkan/commandPool.hpp>
#include <vulkan/device.hpp>
#include <vulkan/geometry.hpp>
#include <vulkan/image.hpp>
#include <vulkan/imageView.hpp>
#include <vulkan/vertex.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define TINYGLTF_USE_CPP14
#include <scene/tiny_gltf.h>

using namespace rayce;
namespace fs = std::filesystem;

RayceScene::RayceScene()
    : mReflectionOpen(true)
{
}

RayceScene::~RayceScene()
{
    for (auto& cached : mImageCache)
    {
        delete[] cached.second;
    }
    mImageCache.clear();
}

static void loadModelMatrix(const tinygltf::Node& node, const tinygltf::Model& model, const mat4& parentTransformation, std::unordered_map<int32, mat4>& transformationMatrices)
{
    mat4 localTransformationMatrix;

    if (node.matrix.size() == 16)
    {
        localTransformationMatrix = Eigen::Map<const Eigen::Matrix4d>(node.matrix.data()).cast<float>();
    }
    else
    {
        vec3 position = makeVec3(0.0f);
        quat rotation = quat(1.0f, 0.0f, 0.0f, 0.0f);
        vec3 scale    = makeVec3(1.0f);
        if (node.translation.size() == 3)
        {
            position = vec3(static_cast<float>(node.translation[0]), static_cast<float>(node.translation[1]), static_cast<float>(node.translation[2]));
        }
        if (node.rotation.size() == 4)
        {
            rotation = quat(static_cast<float>(node.rotation[3]), static_cast<float>(node.rotation[0]), static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2]));
        }
        if (node.scale.size() == 3)
        {
            scale = vec3(static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]), static_cast<float>(node.scale[2]));
        }

        localTransformationMatrix = (Eigen::Translation3f(position) * rotation * Eigen::Scaling(scale)).matrix();
    }

    mat4 globalTransformationMatrix = parentTransformation * localTransformationMatrix;

    if (node.mesh >= 0)
    {
        transformationMatrices[node.mesh] = globalTransformationMatrix;
    }

    for (ptr_size i = 0; i < node.children.size(); ++i)
    {
        RAYCE_ASSERT(node.children[i] < static_cast<int32>(model.nodes.size()), "Invalid gltf node!");

        loadModelMatrix(model.nodes[node.children[i]], model, globalTransformationMatrix, transformationMatrices);
    }
}

void RayceScene::loadFromGltf(const str& filename, const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<CommandPool>& commandPool, float scale)
{
    str ext = filename.substr(filename.find_last_of(".") + 1);

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    str err;
    str warn;
    bool ret = false;
    if (ext.compare("glb") == 0)
    {
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename.c_str());
    }
    else
    {
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename.c_str());
    }

    if (!warn.empty())
    {
        RAYCE_LOG_ERROR("TinyGLTF warning: %s", warn.c_str());
    }

    if (!err.empty())
    {
        RAYCE_LOG_ERROR("TinyGLTF error: %s", err.c_str());
    }

    if (!ret)
    {
        RAYCE_LOG_ERROR("Can not load: %s", filename.c_str());
        return;
    }

    RAYCE_LOG_INFO("Loading GLTF model %s.", filename.c_str());

    mReflectionInfo.filename = filename;

    pGeometry = std::make_unique<Geometry>();

    // first get transformation matrices for the meshes
    std::unordered_map<int32, mat4> transformationMatrices;

    for (const tinygltf::Scene& scene : model.scenes)
    {
        for (int32 i = 0; i < static_cast<int32>(scene.nodes.size()); ++i)
        {
            tinygltf::Node node = model.nodes.at(scene.nodes[i]);
            loadModelMatrix(node, model, mat4::Identity(), transformationMatrices);
        }
    }

    uint32 meshId = 0;
    for (const auto& mesh : model.meshes)
    {
        mReflectionInfo.meshNames.push_back(mesh.name);
        mReflectionInfo.meshTriCounts.push_back(0);
        RAYCE_LOG_INFO("Loading mesh %s.", mesh.name.c_str());

        for (const auto& primitive : mesh.primitives)
        {
            // 1 meshPrimitive = 1 BLAS later -> buffers per meshPrimitive
            std::vector<Vertex> vertices;
            std::vector<uint32> indices;
            // indices
            {
                const auto& iAccessor   = model.accessors[primitive.indices];
                const auto& bufferView  = model.bufferViews[iAccessor.bufferView];
                const auto& buffer      = model.buffers[bufferView.buffer];
                const byte* dataStart   = buffer.data.data() + bufferView.byteOffset + iAccessor.byteOffset;
                const uint32 byteStride = static_cast<uint32>(iAccessor.ByteStride(bufferView));
                const uint32 count      = static_cast<uint32>(iAccessor.count);

                // we want uint32
                switch (iAccessor.componentType)
                {
                case TINYGLTF_COMPONENT_TYPE_BYTE:
                    extendVector(indices, convertGltfDataUint32<char, 1>(dataStart, count, byteStride));
                    break;

                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    extendVector(indices, convertGltfDataUint32<unsigned char, 1>(dataStart, count, byteStride));
                    break;

                case TINYGLTF_COMPONENT_TYPE_SHORT:
                    extendVector(indices, convertGltfDataUint32<short, 1>(dataStart, count, byteStride));
                    break;

                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    extendVector(indices, convertGltfDataUint32<unsigned short, 1>(dataStart, count, byteStride));
                    break;

                case TINYGLTF_COMPONENT_TYPE_INT:
                    extendVector(indices, convertGltfDataUint32<int, 1>(dataStart, count, byteStride));
                    break;

                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    extendVector(indices, convertGltfDataUint32<unsigned int, 1>(dataStart, count, byteStride));
                    break;
                default:
                    break;
                }
            }

            // attributes
            {
                // convert to triangle list
                bool converted = (primitive.mode == TINYGLTF_MODE_TRIANGLES);
                switch (primitive.mode)
                {
                case TINYGLTF_MODE_TRIANGLE_FAN:
                    if (!converted)
                    {
                        std::vector<uint32> triangleFan = std::move(indices);
                        indices.clear();
                        for (ptr_size i = 2; i < triangleFan.size(); ++i)
                        {
                            indices.push_back(triangleFan[0]);
                            indices.push_back(triangleFan[i - 1]);
                            indices.push_back(triangleFan[i]);
                        }
                        converted = true;
                    }
                case TINYGLTF_MODE_TRIANGLE_STRIP:
                    if (!converted)
                    {
                        std::vector<uint32> triangleStrip = std::move(indices);
                        indices.clear();
                        for (ptr_size i = 2; i < triangleStrip.size(); ++i)
                        {
                            indices.push_back(triangleStrip[i - 2]);
                            indices.push_back(triangleStrip[i - 1]);
                            indices.push_back(triangleStrip[i]);
                        }
                        converted = true;
                    }
                case TINYGLTF_MODE_TRIANGLES:
                    break;
                default:
                    RAYCE_LOG_ERROR("Primitive Mode not supported!");
                    return;
                }

                if (converted)
                {
                    std::vector<vec3> positions;
                    std::vector<vec3> normals;
                    std::vector<vec2> uvs;
                    uint32 materialId = primitive.material; // FIXME: Problem with multiple gltfs.

                    // attributes
                    for (const auto& attribute : primitive.attributes)
                    {
                        const auto aAccessor    = model.accessors[attribute.second];
                        const auto& bufferView  = model.bufferViews[aAccessor.bufferView];
                        const auto& buffer      = model.buffers[bufferView.buffer];
                        const byte* dataStart   = buffer.data.data() + bufferView.byteOffset + aAccessor.byteOffset;
                        const uint32 byteStride = static_cast<uint32>(aAccessor.ByteStride(bufferView));
                        const uint32 count      = static_cast<uint32>(aAccessor.count);
                        if (attribute.first == "POSITION")
                        {
                            // vertices
                            switch (aAccessor.type)
                            {
                            case TINYGLTF_TYPE_VEC3:
                            {
                                switch (aAccessor.componentType)
                                {
                                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                                {
                                    extendVector(positions, convertGltfDataVec3<float, 3>(dataStart, count, byteStride, [scale](const float& entry)
                                                                                          { return entry * scale; }));
                                    break;
                                }
                                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                                {
                                    extendVector(positions, convertGltfDataVec3<double, 3>(dataStart, count, byteStride, [scale](const double& entry)
                                                                                           { return entry * scale; }));
                                    break;
                                }
                                }
                                break;
                            }
                            default:
                                RAYCE_LOG_ERROR("Only VEC3 is supported for positions!");
                                return;
                            }
                        }
                        if (attribute.first == "NORMAL")
                        {
                            // normals
                            switch (aAccessor.type)
                            {
                            case TINYGLTF_TYPE_VEC3:
                            {
                                switch (aAccessor.componentType)
                                {
                                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                                {
                                    extendVector(normals, convertGltfDataVec3<float, 3>(dataStart, count, byteStride));
                                    break;
                                }
                                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                                {
                                    extendVector(normals, convertGltfDataVec3<double, 3>(dataStart, count, byteStride));
                                    break;
                                }
                                }
                                break;
                            }
                            default:
                                RAYCE_LOG_ERROR("Only VEC3 is supported for normals!");
                                return;
                            }
                        }
                        if (attribute.first == "TEXCOORD_0")
                        {
                            // normals
                            switch (aAccessor.type)
                            {
                            case TINYGLTF_TYPE_VEC2:
                            {
                                switch (aAccessor.componentType)
                                {
                                case TINYGLTF_COMPONENT_TYPE_FLOAT:
                                {
                                    extendVector(uvs, convertGltfDataVec2<float, 2>(dataStart, count, byteStride));
                                    break;
                                }
                                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                                {
                                    extendVector(uvs, convertGltfDataVec2<double, 2>(dataStart, count, byteStride));
                                    break;
                                }
                                }
                                break;
                            }
                            default:
                                RAYCE_LOG_ERROR("Only VEC2 is supported for uvs!");
                                return;
                            }
                        }
                    }

                    vertices.resize(positions.size());
                    for (ptr_size v = 0; v < positions.size(); ++v)
                    {
                        Vertex vertex;

                        vertex.position = positions[v];
                        vertex.normal   = normals[v];
                        vertex.uv       = uvs[v];

                        vertices[v] = vertex;
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

                    uint32 maxVertex      = static_cast<uint32>(vertices.size() - 1);
                    uint32 primitiveCount = static_cast<uint32>(indices.size() / 3);
                    mReflectionInfo.meshTriCounts.back() += primitiveCount;

                    pGeometry->add(std::move(vertexBuffer), maxVertex, std::move(indexBuffer), primitiveCount, materialId, transformationMatrices[meshId]);
                }
            }
        }
        meshId++;
    }

    // materials and textures
    {
        std::vector<bool> mSrgb(model.textures.size(), false);
        for (const auto& material : model.materials)
        {
            RAYCE_LOG_INFO("Loading material %s.", material.name.c_str());

            Material mat;

            auto& pbr = material.pbrMetallicRoughness;

            if (pbr.baseColorTexture.index < 0)
            {
                auto& col     = pbr.baseColorFactor;
                mat.baseColor = vec4(static_cast<float>(col[0]), static_cast<float>(col[1]), static_cast<float>(col[2]), static_cast<float>(col[3]));
            }
            else
            {
                mat.baseColorTextureId        = pbr.baseColorTexture.index;
                mSrgb[mat.baseColorTextureId] = true;
            }

            if (pbr.metallicRoughnessTexture.index < 0)
            {
                mat.metallicFactor  = static_cast<float>(pbr.metallicFactor);
                mat.roughnessFactor = static_cast<float>(pbr.roughnessFactor);
            }
            else
            {
                mat.metallicRoughnessTextureId = pbr.metallicRoughnessTexture.index;
            }

            if (material.emissiveTexture.index < 0)
            {
                auto& col         = material.emissiveFactor;
                mat.emissiveColor = vec3(static_cast<float>(col[0]), static_cast<float>(col[1]), static_cast<float>(col[2]));
            }
            else
            {
                mat.emissiveTextureId         = material.emissiveTexture.index;
                mSrgb[mat.baseColorTextureId] = true;
            }

            if (material.normalTexture.index > 0)
            {
                mat.normalTextureId = material.normalTexture.index;
            }

            mMaterials.push_back(std::make_unique<Material>(mat));
        }

        uint32 modelTexCount = 0;
        for (ptr_size i = 0; i < model.textures.size(); ++i)
        {
            const auto& texture = model.textures[i];

            str name = texture.name;

            if (name.empty())
            {
                name = filename + "_tex" + std::to_string(modelTexCount++);
            }

            if (mImageCache[name])
            {
                continue;
            }

            RAYCE_LOG_INFO("Loading texture %s.", name.c_str());

            const auto& image = model.images[texture.source];

            uint32 width      = static_cast<uint32>(image.width);
            uint32 height     = static_cast<uint32>(image.height);
            uint32 components = static_cast<uint32>(image.component);
            uint32 imageSize  = width * height * components;

            mImageCache[name] = new byte[imageSize];
            memcpy(mImageCache[name], image.image.data(), imageSize);

            VkFormat format = getImageFormat(components, mSrgb[i]);

            VkExtent2D extent{ width, height };
            mImages.push_back(std::make_unique<Image>(logicalDevice, extent, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
            auto& addedImage = mImages.back();
            addedImage->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
            VkExtent3D extent3D{ width, height, 1 };
            Image::uploadImageDataWithStagingBuffer(logicalDevice, commandPool, *addedImage, mImageCache[name], imageSize, extent3D);
            addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            mImageViews.push_back(std::make_unique<ImageView>(logicalDevice, *addedImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
        }
    }
}

void RayceScene::onImGuiRender()
{
    if (!ImGui::Begin("Scene Reflection", nullptr, 0))
    {
        ImGui::End();
        return;
    }

    ImGui::Spacing();
    static ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (ImGui::TreeNodeEx(mReflectionInfo.filename.c_str(), treeNodeFlags, "%s  %s", ICON_FA_FOLDER, mReflectionInfo.filename.c_str()))
    {
        for (ptr_size i = 0; i < mReflectionInfo.meshNames.size(); ++i)
        {
            ImGui::Indent();
            ImGui::Text("%s %s: %d Triangles", ICON_FA_SHAPES, mReflectionInfo.meshNames[i].c_str(), mReflectionInfo.meshTriCounts[i]);
            if (i < mReflectionInfo.meshNames.size() - 1)
            {
                ImGui::Separator();
            }
        }
        ImGui::TreePop();
    }

    ImGui::End();
}
