/// @file      scene.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <filesystem>
#include <functional>
#include <host_device.hpp>
#include <imgui.h>
#include <scene/loadHelper.hpp>
#include <scene/rayceScene.hpp>
#include <vulkan/buffer.hpp>
#include <vulkan/commandPool.hpp>
#include <vulkan/device.hpp>
#include <vulkan/geometry.hpp>
#include <vulkan/image.hpp>
#include <vulkan/imageView.hpp>
#include <vulkan/sampler.hpp>

#include <scene/tinyparser-mitsuba.h>

using namespace rayce;
namespace fs = std::filesystem;
namespace mp = TPM_NAMESPACE;

using MitsubaRef = str;
struct MitsubaBSDF
{
    EBSDFType type;
    MitsubaRef id;
    Material possibleData;
};

struct MitsubaEmitter
{
    ELightType type;
    Light possibleData;
};

struct MitsubaShape
{
    str filename;
    mat4 transformationMatrix;
    MitsubaRef bsdf;
    int32 emitter{-1};
};

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

static EBSDFType bsdfFromPluginType(const str& pluginType)
{
    if (pluginType == "diffuse")
    {
        return EBSDFType::diffuse;
    }
    else if (pluginType == "dielectric")
    {
        return EBSDFType::smoothDielectric;
    }
    else if (pluginType == "thindielectric")
    {
        return EBSDFType::smoothDielectricThin;
    }
    else if (pluginType == "roughdielectric")
    {
        return EBSDFType::roughDielectric;
    }
    else if (pluginType == "conductor")
    {
        return EBSDFType::smoothConductor;
    }
    else if (pluginType == "roughconductor")
    {
        return EBSDFType::roughConductor;
    }
    else if (pluginType == "plastic")
    {
        return EBSDFType::smoothPlastic;
    }
    else if (pluginType == "roughplastic")
    {
        return EBSDFType::roughPlastic;
    }
    else if (pluginType == "twosided")
    {
        return EBSDFType::count;
    }
    RAYCE_ASSERT(false, "Unknown BSDF Type");
}

static ELightType lightFromPluginType(const str& pluginType)
{
    if (pluginType == "area")
    {
        return ELightType::area;
    }
    else if (pluginType == "point")
    {
        return ELightType::point;
    }
    else if (pluginType == "constant")
    {
        return ELightType::constant;
    }
    else if (pluginType == "envmap")
    {
        return ELightType::envmap;
    }
    else if (pluginType == "spot")
    {
        return ELightType::spot;
    }
    else if (pluginType == "projector")
    {
        return ELightType::projector;
    }
    else if (pluginType == "directional")
    {
        return ELightType::directional;
    }
    else if (pluginType == "directionalarea")
    {
        return ELightType::directionalArea;
    }
    RAYCE_ASSERT(false, "Unknown BSDF Type");
}

static MitsubaBSDF loadMitsubaBSDF(const std::shared_ptr<tinyparser_mitsuba::Object>& bsdfObject, std::vector<str>& imagesToLoad)
{
    MitsubaBSDF bsdf;
    bsdf.id   = bsdfObject->id();
    bsdf.type = bsdfFromPluginType(bsdfObject->pluginType());

    auto& props = bsdfObject->properties();
    switch (bsdf.type)
    {
    case EBSDFType::count: // twosided adapter
    {
        for (const auto& bsdfChild : bsdfObject->anonymousChildren())
        {
            if (bsdfChild->type() != mp::OT_BSDF)
            {
                continue;
            }

            return loadMitsubaBSDF(bsdfChild, imagesToLoad);
        }
        break;
    }
    case EBSDFType::diffuse:
    {
        if (props.contains("reflectance"))
        {
            auto reflectance = props.at("reflectance");

            if (reflectance.type() == mp::PT_COLOR) // <rgb></rgb>
            {
                auto& c                              = reflectance.getColor();
                bsdf.possibleData.diffuseReflectance = vec3(c.r, c.g, c.b);
            }
            if (reflectance.type() == mp::PT_SPECTRUM) // <spectrum></spectrum>
            {
                RAYCE_LOG_WARN("Spectrum is not supported at the moment!");
            }

            for (const auto& textureChild : bsdfObject->namedChildren())
            {
                // texture
                if (textureChild.second->type() != mp::OT_TEXTURE)
                {
                    continue;
                }
                for (const auto& textureProperty : textureChild.second->properties())
                {
                    if (textureProperty.first == "filename")
                    {
                        bsdf.possibleData.diffuseReflectanceTexture = imagesToLoad.size();
                        imagesToLoad.push_back(textureProperty.second.getString());
                    }
                }
            }
        }
        break;
    }
    default:
        RAYCE_LOG_WARN("Unsupported bsdf!");
        break;
    }

    return bsdf;
}

static MitsubaEmitter loadMitsubaEmitter(const std::shared_ptr<tinyparser_mitsuba::Object>& emitterObject, std::vector<str>& imagesToLoad)
{
    MitsubaEmitter emitter;
    emitter.type = lightFromPluginType(emitterObject->pluginType());

    auto& props = emitterObject->properties();
    switch (emitter.type)
    {
    case ELightType::area:
    {

        if (props.contains("radiance"))
        {
            auto radiance = props.at("radiance");

            if (radiance.type() == mp::PT_COLOR) // <rgb></rgb>
            {
                auto& c                              = radiance.getColor();
                emitter.possibleData.radiance = vec3(c.r, c.g, c.b);
            }
            if (radiance.type() == mp::PT_SPECTRUM) // <spectrum></spectrum>
            {
                RAYCE_LOG_WARN("Spectrum is not supported at the moment!");
            }

            for (const auto& textureChild : emitterObject->namedChildren())
            {
                // texture
                if (textureChild.second->type() != mp::OT_TEXTURE)
                {
                    continue;
                }
                for (const auto& textureProperty : textureChild.second->properties())
                {
                    if (textureProperty.first == "filename")
                    {
                        emitter.possibleData.radianceTexture = imagesToLoad.size();
                        imagesToLoad.push_back(textureProperty.second.getString());
                    }
                }
            }
        }
        break;
    }
    default:
        RAYCE_LOG_WARN("Unsupported bsdf!");
        break;
    }
}

void RayceScene::loadFromMitsubaFile(const str& filename, const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<CommandPool>& commandPool, float scale)
{
    str ext = filename.substr(filename.find_last_of(".") + 1);

    mp::SceneLoader sceneLoader;

    mp::Scene scene = sceneLoader.loadFromFile(filename.c_str());

    RAYCE_LOG_INFO("Loading Mitsuba file %s.", filename.c_str());
    // FIXME: Implement more features - sensor, instances, shapegroups, primitives not loaded from files etc.

    mReflectionInfo.filename = filename;

    std::vector<MitsubaShape> mitsubaShapes;
    std::vector<MitsubaEmitter> mitsubaEmitters;
    std::unordered_map<str, MitsubaBSDF> mitsubaBSDFs;
    std::vector<str> imagesToLoad;
    uint32 inlineBSDFId = 0;

    for (const auto& object : scene.anonymousChildren())
    {
        switch (object->type())
        {
        case mp::OT_SHAPE:
        {
            MitsubaShape shape;
            auto pluginType = object->pluginType();
            if (pluginType == "rectangle")
            {
                shape.filename = "rectangle.ply";
            }
            if (pluginType == "sphere")
            {
                shape.filename = "sphere.ply";
            }
            for (const auto& prop : object->properties())
            {
                if (prop.first == "filename")
                {
                    shape.filename = prop.second.getString();
                }
                else if (prop.first == "to_world")
                {
                    const auto& transform      = prop.second.getTransform();
                    shape.transformationMatrix = mat4::Identity();
                    for (int i = 0; i < 4; i++)
                    {
                        for (int j = 0; j < 4; j++)
                        {
                            shape.transformationMatrix(i, j) = transform(i, j);
                        }
                    }
                }
            }

            for (const auto& child : object->anonymousChildren())
            {
                if (child->type() == mp::OT_BSDF)
                {
                    MitsubaBSDF mbsdf = loadMitsubaBSDF(child, imagesToLoad);

                    if (mbsdf.id.empty())
                    {
                        mbsdf.id = "inline_bsdf_" + inlineBSDFId++; // FIXME: potential collisions
                    }

                    shape.bsdf               = mbsdf.id;
                    mitsubaBSDFs[shape.bsdf] = mbsdf;
                    continue;
                }

                if (child->type() == mp::OT_EMITTER)
                {
                    MitsubaEmitter memitter = loadMitsubaEmitter(child, imagesToLoad);
                    shape.emitter = mitsubaEmitters.size(); // TODO: We have to connect that vi primitiveId later on!
                    mitsubaEmitters.push_back(memitter);
                    continue;
                }
            }
            mitsubaShapes.push_back(shape);
            break;
        }
        case mp::OT_BSDF:
        {

            MitsubaBSDF mbsdf = loadMitsubaBSDF(object, imagesToLoad);

            if (mbsdf.id.empty())
            {
                mbsdf.id = "inline_bsdf_" + inlineBSDFId++; // FIXME: potential collisions
            }

            mitsubaBSDFs[mbsdf.id] = mbsdf;

            break;
        }
        case mp::OT_EMITTER:
        {
            RAYCE_LOG_WARN("Only area lights are supported at the moment!");
            break;
        }
        default:
            break;
        }
    }

    pGeometry = std::make_unique<Geometry>();

    // TODO: NEXT: Add tinyply (+ tinyobj) and load geometry + load textures + connect everything (mesh, material, light)
    mat4 tr = mat4::Identity() * scale;
    for (const tinygltf::Scene& scene : model.scenes)
    {
        for (int32 i = 0; i < static_cast<int32>(scene.nodes.size()); ++i)
        {
            tinygltf::Node node = model.nodes.at(scene.nodes[i]);
            loadModelMatrix(node, model, tr, transformationMatrices);
        }
    }

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
                                    extendVector(positions, convertGltfDataVec3<float, 3>(dataStart, count, byteStride));
                                    break;
                                }
                                case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                                {
                                    extendVector(positions, convertGltfDataVec3<double, 3>(dataStart, count, byteStride));
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
                    bool hasNormals   = !normals.empty();
                    bool hasTexCoords = !uvs.empty();
                    for (ptr_size v = 0; v < positions.size(); ++v)
                    {
                        Vertex vertex;

                        vertex.position = positions[v];
                        vertex.normal   = hasNormals ? normals[v].normalized() : vec3(1.0, 1.0, 1.0).normalized();
                        vertex.uv       = hasTexCoords ? uvs[v] : vec2(1.0, 1.0);

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
            // FIXME: duplicates are loaded multiple times.
            RAYCE_LOG_INFO("Loading material %s.", material.name.c_str());

            Material mat;

            auto& pbr = material.pbrMetallicRoughness;

            mat.hasUV = 0;

            mat.baseColorTextureId = pbr.baseColorTexture.index;

            auto& col     = pbr.baseColorFactor;
            mat.baseColor = vec4(static_cast<float>(col[0]), static_cast<float>(col[1]), static_cast<float>(col[2]), static_cast<float>(col[3]));
            if (pbr.baseColorTexture.index >= 0)
            {
                mSrgb[mat.baseColorTextureId] = true;
                mat.hasUV                     = 1;
            }

            mat.metallicRoughnessTextureId = pbr.metallicRoughnessTexture.index;
            mat.metallicFactor             = static_cast<float>(pbr.metallicFactor);
            mat.roughnessFactor            = static_cast<float>(pbr.roughnessFactor);
            if (pbr.metallicRoughnessTexture.index >= 0)
            {
                mat.hasUV = 1;
            }

            mat.emissiveTextureId          = material.emissiveTexture.index;
            mat.emissiveStrength           = 1.0f;
            auto emissiveStrengthExtension = material.extensions.find("KHR_materials_emissive_strength");
            if (emissiveStrengthExtension != material.extensions.end())
            {
                // FIXME: Handle type error...
                mat.emissiveStrength = emissiveStrengthExtension->second.Get("emissiveStrength").GetNumberAsDouble();
            }

            // emission
            auto& emCol       = material.emissiveFactor;
            mat.emissiveColor = vec3(static_cast<float>(emCol[0]), static_cast<float>(emCol[1]), static_cast<float>(emCol[2]));

            if (material.emissiveTexture.index >= 0)
            {
                mSrgb[mat.baseColorTextureId] = true;
                mat.hasUV                     = 1;
            }

            // normal texture
            mat.normalTextureId = material.normalTexture.index;
            if (material.normalTexture.index >= 0)
            {
                mat.hasUV = 1;
            }

            // transmittance and ior
            mat.transmission           = 0.0f;
            mat.ior                    = 1.5f;
            auto transmissionExtension = material.extensions.find("KHR_materials_transmission");
            auto iorExtension          = material.extensions.find("KHR_materials_ior");
            if (transmissionExtension != material.extensions.end())
            {
                // FIXME: Handle type error...
                // FIXME: Transmission texture?
                mat.transmission = transmissionExtension->second.Get("transmissionFactor").GetNumberAsDouble();
            }
            if (iorExtension != material.extensions.end())
            {
                // FIXME: Handle type error...
                mat.ior = iorExtension->second.Get("ior").GetNumberAsDouble();
            }

            mMaterials.push_back(std::make_unique<Material>(mat));
        }

        uint32 modelTexCount = 0;
        for (ptr_size i = 0; i < model.textures.size(); ++i)
        {
            const auto& texture = model.textures[i];
            const auto& smpler  = model.samplers[texture.sampler];

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

            // FIXME: To function?
            VkFilter magFilter = smpler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
            VkFilter minFilter = (smpler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST || smpler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST ||
                                  smpler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR)
                                     ? VK_FILTER_NEAREST
                                     : VK_FILTER_LINEAR;
            VkSamplerMipmapMode mipmapMode =
                (smpler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR ||
                 smpler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR)
                    ? VK_SAMPLER_MIPMAP_MODE_LINEAR
                    : VK_SAMPLER_MIPMAP_MODE_NEAREST;

            VkSamplerAddressMode addressU =
                (smpler.wrapS == TINYGLTF_TEXTURE_WRAP_REPEAT) ? VK_SAMPLER_ADDRESS_MODE_REPEAT
                                                               : (smpler.wrapS == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
                                                                                                                      : VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT);

            VkSamplerAddressMode addressV =
                (smpler.wrapT == TINYGLTF_TEXTURE_WRAP_REPEAT) ? VK_SAMPLER_ADDRESS_MODE_REPEAT
                                                               : (smpler.wrapS == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
                                                                                                                      : VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT);

            std::unique_ptr<Sampler> sampler = std::make_unique<Sampler>(logicalDevice, magFilter, minFilter, addressU, addressV, VK_SAMPLER_ADDRESS_MODE_REPEAT, mipmapMode, true, false, VK_COMPARE_OP_ALWAYS);
            mImageViews.push_back(std::make_unique<ImageView>(logicalDevice, *addedImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
            mImageSamplers.push_back(std::move(sampler));
        }

        str name = "Default";
        RAYCE_LOG_INFO("Loading texture %s.", name.c_str());

        uint32 width      = 1;
        uint32 height     = 1;
        uint32 components = 1;
        uint32 imageSize  = width * height * components;

        mImageCache[name]    = new byte[imageSize];
        mImageCache[name][0] = 0;

        VkFormat format = getImageFormat(components, false);

        VkExtent2D extent{ width, height };
        mImages.push_back(std::make_unique<Image>(logicalDevice, extent, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
        auto& addedImage = mImages.back();
        addedImage->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VkExtent3D extent3D{ width, height, 1 };
        Image::uploadImageDataWithStagingBuffer(logicalDevice, commandPool, *addedImage, mImageCache[name], imageSize, extent3D);
        addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        std::unique_ptr<Sampler> defaultSampler = std::make_unique<Sampler>(logicalDevice, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                                                                            VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_MIPMAP_MODE_LINEAR, true, false, VK_COMPARE_OP_ALWAYS);
        mImageViews.push_back(std::make_unique<ImageView>(logicalDevice, *addedImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
        mImageSamplers.push_back(std::move(defaultSampler));
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
        ImGui::Indent();
        for (ptr_size i = 0; i < mReflectionInfo.meshNames.size(); ++i)
        {
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
