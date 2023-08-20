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

#include <scene/miniply.h>
#include <scene/tinyparser-mitsuba.h>

#define STB_IMAGE_IMPLEMENTATION
#include <scene/stb_image.h>

using namespace rayce;
namespace fs = std::filesystem;
namespace mp = TPM_NAMESPACE;

using MitsubaRef = str;
struct MitsubaBSDF
{
    EBSDFType type;
    MitsubaRef id;
    Material possibleData;
    int32 materialId{ -1 };
};

struct MitsubaEmitter
{
    ELightType type;
    Light possibleData;
    int32 lightId{ -1 };
};

struct MitsubaShape
{
    str filename;
    mat4 transformationMatrix;
    MitsubaRef bsdf;
    int32 emitter{ -1 };
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
        return EBSDFType::bsdfTypeCount;
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
    case EBSDFType::bsdfTypeCount: // twosided adapter
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
                auto& c                       = radiance.getColor();
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
        RAYCE_LOG_WARN("Unsupported emitter!");
        break;
    }

    return emitter;
}

void RayceScene::loadFromMitsubaFile(const str& filename, const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<CommandPool>& commandPool, float scale)
{

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
            mReflectionInfo.meshNames.push_back(object->id());
            mReflectionInfo.meshTriCounts.push_back(0);
            if (pluginType == "rectangle")
            {
                shape.filename = fs::path(filename).parent_path().concat("\\rectangle.ply").string();
            }
            if (pluginType == "sphere")
            {
                shape.filename = fs::path(filename).parent_path().concat("\\sphere.ply").string();
            }
            for (const auto& prop : object->properties())
            {
                if (prop.first == "filename")
                {
                    shape.filename = fs::path(filename).parent_path().concat("\\" + prop.second.getString()).string(); // FIXME: absolute? relative?
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
                    shape.emitter           = mitsubaEmitters.size();
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

    // TODO: NEXT: Connect everything (mesh, material, light)
    std::unique_ptr<Sampler> defaultSampler = std::make_unique<Sampler>(logicalDevice, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                                                                        VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_MIPMAP_MODE_LINEAR, true, false, VK_COMPARE_OP_ALWAYS);

    for (auto& [ref, bsdf] : mitsubaBSDFs)
    {
        RAYCE_LOG_INFO("Creating material from %s.", ref.c_str());

        switch (bsdf.type)
        {
        case EBSDFType::diffuse:
        {
            if (bsdf.possibleData.diffuseReflectanceTexture >= 0)
            {
                // load image;
                str name = bsdf.id + "_diffuseReflectance";

                if (mImageCache[name])
                {
                    continue;
                }

                str imageFile = imagesToLoad[bsdf.possibleData.diffuseReflectanceTexture];

                int32 w, h, c;

                if (!fs::exists(imageFile))
                {
                    imageFile = fs::path(filename).parent_path().concat("\\" + imageFile).string();
                    if (!fs::exists(imageFile))
                    {
                        RAYCE_LOG_ERROR("Can not find %s nor %s", imagesToLoad[bsdf.possibleData.diffuseReflectanceTexture].c_str(), imageFile.c_str());
                    }
                }

                mImageCache[name] =
                    stbi_load(imageFile.c_str(), &w, &h, &c, STBI_rgb_alpha);
                if (!mImageCache[name])
                {
                    RAYCE_LOG_ERROR("Can not load: %s", imageFile.c_str());
                }
                RAYCE_LOG_INFO("Loaded %s as %s", imageFile.c_str(), name.c_str());

                uint32 width      = static_cast<uint32>(w);
                uint32 height     = static_cast<uint32>(h);
                uint32 components = static_cast<uint32>(c);
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

                mImageViews.push_back(std::make_unique<ImageView>(logicalDevice, *addedImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
                mImageSamplers.push_back(std::move(defaultSampler));
            }
            break;
        }
        default:
            RAYCE_LOG_WARN("Unsupported bsdf!");
            break;
        }

        bsdf.materialId = mMaterials.size();
        mMaterials.push_back(std::make_unique<Material>(bsdf.possibleData));
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

    mImageViews.push_back(std::make_unique<ImageView>(logicalDevice, *addedImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
    mImageSamplers.push_back(std::move(defaultSampler));

    // emitters -> lights
    int32 emitterId = 0;
    for (auto& emitter : mitsubaEmitters)
    {
        RAYCE_LOG_INFO("Creating light from emitter %d.", emitterId);

        switch (emitter.type)
        {
        case ELightType::area:
        {
            if (emitter.possibleData.radianceTexture >= 0)
            {
                // load image;
                str name = "emitter_" + std::to_string(emitterId) + "_radiance";

                if (mImageCache[name])
                {
                    continue;
                }

                str imageFile = imagesToLoad[emitter.possibleData.radianceTexture];

                int32 w, h, c;

                if (!fs::exists(imageFile))
                {
                    imageFile = fs::path(filename).parent_path().concat("\\" + imageFile).string();
                    if (!fs::exists(imageFile))
                    {
                        RAYCE_LOG_ERROR("Can not find %s nor %s", imagesToLoad[emitter.possibleData.radianceTexture].c_str(), imageFile.c_str());
                    }
                }

                mImageCache[name] =
                    stbi_load(imageFile.c_str(), &w, &h, &c, STBI_rgb_alpha);
                if (!mImageCache[name])
                {
                    RAYCE_LOG_ERROR("Can not load: %s", imageFile.c_str());
                }
                RAYCE_LOG_INFO("Loaded %s as %s", imageFile.c_str(), name.c_str());

                uint32 width      = static_cast<uint32>(w);
                uint32 height     = static_cast<uint32>(h);
                uint32 components = static_cast<uint32>(c);
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

                mImageViews.push_back(std::make_unique<ImageView>(logicalDevice, *addedImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
                mImageSamplers.push_back(std::move(defaultSampler));
            }
            break;
        }
        default:
            RAYCE_LOG_WARN("Unsupported emitter!");
            break;
        }

        emitter.lightId = mLights.size();
        mLights.push_back(std::make_unique<Light>(emitter.possibleData));

        emitterId++;
    }

    // shapes -> meshes
    uint32 meshId = 0;
    for (auto& shape : mitsubaShapes)
    {
        str ext = shape.filename.substr(shape.filename.find_last_of(".") + 1);

        if (ext == "ply")
        {
            RAYCE_LOG_INFO("Loading: %s.", shape.filename.c_str());
            miniply::PLYReader plyReader(shape.filename.c_str());

            if (!plyReader.valid())
            {
                RAYCE_LOG_ERROR("Can not load: %s!", shape.filename.c_str());
                continue;
            }

            bool hasPositions = false, hasIndices = false, hasNormals = false, hasUVs = false;
            uint32 positionIndices[3];
            uint32 normalIndices[3];
            uint32 uvIndices[2];
            uint32 indexIndex;
            std::vector<uint32> indices;
            std::vector<vec3> positions;
            std::vector<vec3> normals;
            std::vector<vec2> uvs;

            while (plyReader.has_element() && (!hasPositions || !hasIndices || !hasNormals || !hasUVs))
            {
                if (plyReader.element_is(miniply::kPLYVertexElement))
                {
                    if (!plyReader.load_element())
                    {
                        RAYCE_LOG_ERROR("Can not load index element. Canceling the loading process!");
                        break;
                    }

                    if (!plyReader.find_pos(positionIndices))
                    {
                        RAYCE_LOG_ERROR("Can not find position properties. Canceling the loading process!");
                        break;
                    }

                    hasPositions = true;
                    positions.resize(plyReader.num_rows());
                    plyReader.extract_properties(positionIndices, 3, miniply::PLYPropertyType::Float, positions.data());

                    if (plyReader.find_normal(normalIndices))
                    {
                        hasNormals = true;
                        normals.resize(plyReader.num_rows());
                        plyReader.extract_properties(normalIndices, 3, miniply::PLYPropertyType::Float, normals.data());
                    }

                    if (plyReader.find_texcoord(uvIndices))
                    {
                        hasUVs = true;
                        uvs.resize(plyReader.num_rows());
                        plyReader.extract_properties(uvIndices, 2, miniply::PLYPropertyType::Float, uvs.data());
                    }
                }
                else if (!hasIndices && plyReader.element_is(miniply::kPLYFaceElement))
                {
                    if (!plyReader.load_element())
                    {
                        RAYCE_LOG_ERROR("Can not load face element. Canceling the loading process!");
                        break;
                    }

                    if (!plyReader.find_indices(&indexIndex))
                    {
                        RAYCE_LOG_ERROR("Can not find index properties. Canceling the loading process!");
                        break;
                    }

                    hasIndices = true;

                    bool hasPolygons = plyReader.requires_triangulation(indexIndex);

                    if (hasPolygons)
                    {
                        if (!hasPositions)
                        {
                            RAYCE_LOG_ERROR("Can not triangulate before finding vertex data. Canceling the loading process!");
                            break;
                        }
                        indices.resize(plyReader.num_triangles(indexIndex) * 3);
                        plyReader.extract_triangles(indexIndex, reinterpret_cast<float*>(positions.data()), positions.size(), miniply::PLYPropertyType::Int, indices.data());
                    }
                    else
                    {
                        indices.resize(plyReader.num_rows() * 3);
                        plyReader.extract_list_property(indexIndex, miniply::PLYPropertyType::Int, indices.data());
                    }
                }

                plyReader.next_element();
            }

            if (!(hasPositions && hasIndices))
            {
                RAYCE_LOG_ERROR("%s has no positions or indices!", shape.filename.c_str());
                continue;
            }

            std::vector<Vertex> vertices(positions.size());
            for (ptr_size v = 0; v < positions.size(); ++v)
            {
                Vertex vertex;

                vertex.position = positions[v];
                vertex.normal   = hasNormals ? normals[v].normalized() : vec3(1.0, 1.0, 1.0).normalized();
                vertex.uv       = hasUVs ? uvs[v] : vec2(1.0, 1.0);

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
            mReflectionInfo.meshTriCounts[meshId] += primitiveCount;

            // materialId is filled before
            // lightId is filled before
            uint32 materialId = mitsubaBSDFs[shape.bsdf].materialId;
            int32 lightId     = -1;
            if (shape.emitter >= 0)
            {
                lightId                         = mitsubaEmitters[shape.emitter].lightId;
                mLights[lightId]->triangleCount = primitiveCount;
                mLights[lightId]->primitiveId   = meshId;
                mLights[lightId]->objectToWorld = shape.transformationMatrix;
            }
            mMaterials[materialId]->canUseUv = hasUVs;
            pGeometry->add(std::move(vertexBuffer), maxVertex, std::move(indexBuffer), primitiveCount, materialId, lightId, { shape.transformationMatrix });
        }

        meshId++;
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
