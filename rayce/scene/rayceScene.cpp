/// @file      scene.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <cctype>
#include <filesystem>
#include <functional>
#include <hostDeviceInterop.slang>
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
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <scene/tinyparser-mitsuba.h>

#define STB_IMAGE_IMPLEMENTATION
#include <scene/stb_image.h>

#include "core/spectrum.hpp"

using namespace rayce;
namespace fs = std::filesystem;
namespace mp = TPM_NAMESPACE;

using MitsubaRef = str;
struct MitsubaBSDF
{
    EBxDFType type;
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
    EShapeType type;
    str filename;
    mat4 transformationMatrix;
    bool faceNormals;
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

static EBxDFType bsdfFromPluginType(const str& pluginType)
{
    if (pluginType == "diffuse")
    {
        return EBxDFType::lambertDiffuse;
    }
    else if (pluginType == "dielectric")
    {
        return EBxDFType::smoothDielectric;
    }
    else if (pluginType == "thindielectric")
    {
        return EBxDFType::smoothDielectricThin;
    }
    else if (pluginType == "roughdielectric")
    {
        return EBxDFType::roughDielectric;
    }
    else if (pluginType == "conductor")
    {
        return EBxDFType::smoothConductor;
    }
    else if (pluginType == "roughconductor")
    {
        return EBxDFType::roughConductor;
    }
    else if (pluginType == "plastic")
    {
        return EBxDFType::smoothPlastic;
    }
    else if (pluginType == "roughplastic")
    {
        return EBxDFType::roughPlastic;
    }
    else if (pluginType == "twosided")
    {
        return EBxDFType::bsdfTypeCount;
    }
    else
    {
        RAYCE_LOG_WARN("We do not support BSDF Type %s!", pluginType.c_str());
        return EBxDFType::bsdfTypeCount;
    }
}

static ELightType lightFromPluginType(const str& pluginType)
{
    if (pluginType == "area")
    {
        return ELightType::area;
    }
    else if (pluginType == "point")
    {
        RAYCE_LOG_ERROR("Light type %s is not supported at the moment!", pluginType.c_str());
        return ELightType::lightTypeCount;
    }
    else if (pluginType == "constant")
    {
        RAYCE_LOG_ERROR("Light type %s is not supported at the moment!", pluginType.c_str());
        return ELightType::lightTypeCount;
    }
    else if (pluginType == "envmap")
    {
        RAYCE_LOG_ERROR("Light type %s is not supported at the moment!", pluginType.c_str());
        return ELightType::lightTypeCount;
    }
    else if (pluginType == "spot")
    {
        RAYCE_LOG_ERROR("Light type %s is not supported at the moment!", pluginType.c_str());
        return ELightType::lightTypeCount;
    }
    else if (pluginType == "projector")
    {
        RAYCE_LOG_ERROR("Light type %s is not supported at the moment!", pluginType.c_str());
        return ELightType::lightTypeCount;
    }
    else if (pluginType == "directional")
    {
        RAYCE_LOG_ERROR("Light type %s is not supported at the moment!", pluginType.c_str());
        return ELightType::lightTypeCount;
    }
    else if (pluginType == "directionalarea")
    {
        RAYCE_LOG_ERROR("Light type %s is not supported at the moment!", pluginType.c_str());
        return ELightType::lightTypeCount;
    }
    else
    {
        RAYCE_LOG_ERROR("Light type %s is not supported at the moment!", pluginType.c_str());
        return ELightType::lightTypeCount;
    }
}

struct IOREntry
{
    const char* name;
    float value;
};
// from mitsuba
static IOREntry iorData[] = {
    { "vacuum", 1.0f },
    { "helium", 1.000036f },
    { "hydrogen", 1.000132f },
    { "air", 1.000277f },
    { "carbon dioxide", 1.00045f },
    //////////////////////////////////////
    { "water", 1.3330f },
    { "acetone", 1.36f },
    { "ethanol", 1.361f },
    { "carbon tetrachloride", 1.461f },
    { "glycerol", 1.4729f },
    { "benzene", 1.501f },
    { "silicone oil", 1.52045f },
    { "bromine", 1.661f },
    //////////////////////////////////////
    { "water ice", 1.31f },
    { "fused quartz", 1.458f },
    { "pyrex", 1.470f },
    { "acrylic glass", 1.49f },
    { "polypropylene", 1.49f },
    { "bk7", 1.5046f },
    { "sodium chloride", 1.544f },
    { "amber", 1.55f },
    { "pet", 1.5750f },
    { "diamond", 2.419f },

    { NULL, 0.0f }
};

static float iorFromString(str materialName)
{
    std::transform(materialName.begin(), materialName.end(), materialName.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });

    IOREntry* ior = iorData;

    while (ior->name)
    {
        if (materialName == ior->name)
            return ior->value;
        ++ior;
    }

    RAYCE_LOG_ERROR("Can not find an IOR value for %s!", materialName.c_str());

    return 0.0f;
}

struct ComplexIOR
{
    const char* name;
};

static ComplexIOR complexIORData[] = {
    { "a-C" },
    { "Na_palik" },
    { "Ag" },
    { "Nb" },
    { "Nb_palik" },
    { "Al" },
    { "Ni_palik" },
    { "AlAs" },
    { "AlAs_palik" },
    { "Rh" },
    { "Rh_palik" },
    { "AlSb" },
    { "AlSb_palik" },
    { "Se" },
    { "Se_palik" },
    { "Au" },
    { "SiC" },
    { "SiC_palik" },
    { "Be" },
    { "Be_palik" },
    { "SnTe" },
    { "SnTe_palik" },
    { "Cr" },
    { "Ta" },
    { "Ta_palik" },
    { "CsI" },
    { "CsI_palik" },
    { "Te" },
    { "Te_palik" },
    { "Cu" },
    { "Cu_palik" },
    { "ThF4" },
    { "ThF4_palik" },
    { "Cu2O" },
    { "Cu2O_palik" },
    { "TiC" },
    { "TiC_palik" },
    { "CuO" },
    { "CuO_palik" },
    { "TiN" },
    { "TiN_palik" },
    { "d-C" },
    { "d-C_palik" },
    { "TiO2" },
    { "TiO2_palik" },
    { "Hg" },
    { "Hg_palik" },
    { "VC" },
    { "VC_palik" },
    { "HgTe" },
    { "HgTe_palik" },
    { "V_palik" },
    { "Ir" },
    { "Ir_palik" },
    { "VN" },
    { "VN_palik" },
    { "K" },
    { "K_palik" },
    { "W" },
    { "Li" },
    { "Li_palik" },
    { "MgO" },
    { "MgO_palik" },
    { "Mo" },
    { "Mo_palik" }
};

static std::tuple<vec3, vec3> conductorComplexIorFromString(str materialName)
{
    if (materialName == "none")
    {
        return { vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0) };
    }

    ComplexIOR* ior = complexIORData;

    while (ior->name)
    {
        if (materialName == ior->name)
        {
            vec3 rgbEta = spectrumToRGB(LinearInterpolatedSpectrum::fromFile(str("assets/spectra/") + materialName + str(".eta.spd")));
            vec3 rgbK   = spectrumToRGB(LinearInterpolatedSpectrum::fromFile(str("assets/spectra/") + materialName + str(".k.spd")));

            return { rgbEta, rgbK };
        }
        ++ior;
    }

    RAYCE_LOG_ERROR("Can not find an IOR value for %s!", materialName.c_str());

    return { vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0) };
}

static LinearInterpolatedSpectrum convertMitsubaSpectrumToLIS(const tinyparser_mitsuba::Spectrum& spec)
{
    auto wvl = spec.wavelengths();
    auto wts = spec.weights();

    assert(wvl.size() == wts.size());

    std::vector<float> wavelengths(wvl.size());
    std::vector<float> values(wts.size());

    for (ptr_size i = 0; i < wvl.size(); ++i)
    {
        wavelengths[i] = wvl[i];
        values[i]      = wts[i];
    }

    LinearInterpolatedSpectrum spectrum = LinearInterpolatedSpectrum(wavelengths, values);

    return spectrum;
}

static MitsubaBSDF loadMitsubaBSDF(const std::shared_ptr<tinyparser_mitsuba::Object>& bsdfObject, std::vector<str>& imagesToLoad, bool twoSided = false)
{
    MitsubaBSDF bsdf;
    bsdf.id                    = bsdfObject->id();
    bsdf.type                  = bsdfFromPluginType(bsdfObject->pluginType());
    bsdf.possibleData.twoSided = twoSided;
    bsdf.possibleData.bxdfType = bsdf.type;

    auto& props = bsdfObject->properties();
    switch (bsdf.type)
    {
    case EBxDFType::bsdfTypeCount: // twosided adapter
    {
        for (const auto& bsdfChild : bsdfObject->anonymousChildren())
        {
            if (bsdfChild->type() != mp::OT_BSDF)
            {
                continue;
            }

            auto inlineBSDF = loadMitsubaBSDF(bsdfChild, imagesToLoad, true);

            if (inlineBSDF.id.empty())
            {
                inlineBSDF.id = bsdf.id;
            }

            return inlineBSDF;
        }
        break;
    }
    case EBxDFType::lambertDiffuse:
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
                auto& spec = reflectance.getSpectrum();

                LinearInterpolatedSpectrum spectrum = convertMitsubaSpectrumToLIS(spec);

                bsdf.possibleData.diffuseReflectance = spectrumToRGB(spectrum);
            }
        }

        for (const auto& textureChild : bsdfObject->namedChildren())
        {
            // texture
            if (textureChild.first != "reflectance" || textureChild.second->type() != mp::OT_TEXTURE)
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

        break;
    }
    case EBxDFType::smoothDielectric:
    case EBxDFType::smoothDielectricThin:
    case EBxDFType::roughDielectric:
    {
        if (props.contains("int_ior"))
        {
            auto interiorIor = props.at("int_ior");

            if (interiorIor.type() == mp::PT_NUMBER) // <float></float>
            {
                bsdf.possibleData.interiorIor = interiorIor.getNumber();
            }
            if (interiorIor.type() == mp::PT_STRING) // <string></string>
            {
                bsdf.possibleData.interiorIor = iorFromString(interiorIor.getString());
            }
        }
        if (props.contains("ext_ior"))
        {
            auto exteriorIor = props.at("ext_ior");

            if (exteriorIor.type() == mp::PT_NUMBER) // <float></float>
            {
                bsdf.possibleData.exteriorIor = exteriorIor.getNumber();
            }
            if (exteriorIor.type() == mp::PT_STRING) // <string></string>
            {
                bsdf.possibleData.exteriorIor = iorFromString(exteriorIor.getString());
            }
        }
        if (props.contains("specular_reflectance"))
        {
            auto specularReflectance = props.at("specular_reflectance");

            if (specularReflectance.type() == mp::PT_COLOR) // <rgb></rgb>
            {
                auto& c                               = specularReflectance.getColor();
                bsdf.possibleData.specularReflectance = vec3(c.r, c.g, c.b);
            }
            if (specularReflectance.type() == mp::PT_SPECTRUM) // <spectrum></spectrum>
            {
                auto& spec = specularReflectance.getSpectrum();

                LinearInterpolatedSpectrum spectrum = convertMitsubaSpectrumToLIS(spec);

                bsdf.possibleData.specularReflectance = spectrumToRGB(spectrum);
            }
        }
        if (props.contains("specular_transmittance"))
        {
            auto specularTransmittance = props.at("specular_transmittance");

            if (specularTransmittance.type() == mp::PT_COLOR) // <rgb></rgb>
            {
                auto& c                                 = specularTransmittance.getColor();
                bsdf.possibleData.specularTransmittance = vec3(c.r, c.g, c.b);
            }
            if (specularTransmittance.type() == mp::PT_SPECTRUM) // <spectrum></spectrum>
            {
                auto& spec = specularTransmittance.getSpectrum();

                LinearInterpolatedSpectrum spectrum = convertMitsubaSpectrumToLIS(spec);

                bsdf.possibleData.specularTransmittance = spectrumToRGB(spectrum);
            }
        }
        for (const auto& textureChild : bsdfObject->namedChildren())
        {
            // texture
            if (textureChild.second->type() != mp::OT_TEXTURE)
            {
                continue;
            }
            if (textureChild.first == "specular_reflectance")
            {
                for (const auto& textureProperty : textureChild.second->properties())
                {
                    if (textureProperty.first == "filename")
                    {
                        bsdf.possibleData.specularReflectanceTexture = imagesToLoad.size();
                        imagesToLoad.push_back(textureProperty.second.getString());
                    }
                }
            }
            if (textureChild.first == "specular_transmittance")
            {
                for (const auto& textureProperty : textureChild.second->properties())
                {
                    if (textureProperty.first == "filename")
                    {
                        bsdf.possibleData.specularTransmittanceTexture = imagesToLoad.size();
                        imagesToLoad.push_back(textureProperty.second.getString());
                    }
                }
            }
        }

        if (bsdf.type == EBxDFType::roughDielectric)
        {
            // alpha, alpha_u, alpha_v
            if (props.contains("alpha"))
            {
                auto alpha = props.at("alpha");

                if (alpha.type() == mp::PT_NUMBER) // <float></float>
                {
                    bsdf.possibleData.alpha = vec3(alpha.getNumber(), alpha.getNumber(), 0.0);
                }
                else
                {
                    bsdf.possibleData.alpha = vec3(1.0, 1.0, 0.0);
                }
            }
            if (props.contains("alpha_u"))
            {
                auto alphaU = props.at("alpha_u");

                if (alphaU.type() == mp::PT_NUMBER) // <float></float>
                {
                    bsdf.possibleData.alpha.x() = alphaU.getNumber();
                }
                else
                {
                    bsdf.possibleData.alpha.x() = 1.0;
                }
            }
            if (props.contains("alpha_v"))
            {
                auto alphaV = props.at("alpha_v");

                if (alphaV.type() == mp::PT_NUMBER) // <float></float>
                {
                    bsdf.possibleData.alpha.y() = alphaV.getNumber();
                }
                else
                {
                    bsdf.possibleData.alpha.y() = 1.0;
                }
            }
            for (const auto& textureChild : bsdfObject->namedChildren())
            {
                // texture
                if (textureChild.first != "alpha" || textureChild.second->type() != mp::OT_TEXTURE)
                {
                    continue;
                }
                for (const auto& textureProperty : textureChild.second->properties())
                {
                    if (textureProperty.first == "filename")
                    {
                        bsdf.possibleData.alphaTexture = imagesToLoad.size();
                        imagesToLoad.push_back(textureProperty.second.getString());
                    }
                }
            }
        }
        break;
    }
    case EBxDFType::smoothConductor:
    case EBxDFType::roughConductor:
    {
        if (props.contains("specular_reflectance"))
        {
            auto specularReflectance = props.at("specular_reflectance");

            if (specularReflectance.type() == mp::PT_COLOR) // <rgb></rgb>
            {
                auto& c                               = specularReflectance.getColor();
                bsdf.possibleData.specularReflectance = vec3(c.r, c.g, c.b);
            }
            if (specularReflectance.type() == mp::PT_SPECTRUM) // <spectrum></spectrum>
            {
                auto& spec = specularReflectance.getSpectrum();

                LinearInterpolatedSpectrum spectrum = convertMitsubaSpectrumToLIS(spec);

                bsdf.possibleData.specularReflectance = spectrumToRGB(spectrum);
            }
        }

        if (props.contains("material"))
        {
            auto material = props.at("material");

            if (material.type() == mp::PT_STRING) // <string></string>
            {
                auto complexIor                = conductorComplexIorFromString(material.getString());
                bsdf.possibleData.conductorEta = std::get<0>(complexIor);
                bsdf.possibleData.conductorK   = std::get<1>(complexIor);
            }
        }
        if (props.contains("eta"))
        {
            auto eta = props.at("eta");

            if (eta.type() == mp::PT_SPECTRUM) // <spectrum></spectrum>
            {
                auto& spec = eta.getSpectrum();

                LinearInterpolatedSpectrum spectrum = convertMitsubaSpectrumToLIS(spec);

                bsdf.possibleData.conductorEta = spectrumToRGB(spectrum);
            }
        }
        if (props.contains("k"))
        {
            auto k = props.at("k");

            if (k.type() == mp::PT_SPECTRUM) // <spectrum></spectrum>
            {
                auto& spec = k.getSpectrum();

                LinearInterpolatedSpectrum spectrum = convertMitsubaSpectrumToLIS(spec);

                bsdf.possibleData.conductorK = spectrumToRGB(spectrum);
            }
        }
        for (const auto& textureChild : bsdfObject->namedChildren())
        {
            // texture
            if (textureChild.second->type() != mp::OT_TEXTURE)
            {
                continue;
            }
            if (textureChild.first == "specular_reflectance")
            {
                for (const auto& textureProperty : textureChild.second->properties())
                {
                    if (textureProperty.first == "filename")
                    {
                        bsdf.possibleData.specularReflectanceTexture = imagesToLoad.size();
                        imagesToLoad.push_back(textureProperty.second.getString());
                    }
                }
            }
            /*
            if (textureChild.first == "") // eta and k
            {
                for (const auto& textureProperty : textureChild.second->properties())
                {
                    if (textureProperty.first == "filename")
                    {
                        bsdf.possibleData.specularTransmittanceTexture = imagesToLoad.size();
                        imagesToLoad.push_back(textureProperty.second.getString());
                    }
                }
            }
            */
        }

        if (bsdf.type == EBxDFType::roughConductor)
        {
            // alpha, alpha_u, alpha_v
            if (props.contains("alpha"))
            {
                auto alpha = props.at("alpha");

                if (alpha.type() == mp::PT_NUMBER) // <float></float>
                {
                    bsdf.possibleData.alpha = vec3(alpha.getNumber(), alpha.getNumber(), 0.0);
                }
                else
                {
                    bsdf.possibleData.alpha = vec3(1.0, 1.0, 0.0);
                }
            }
            if (props.contains("alpha_u"))
            {
                auto alphaU = props.at("alpha_u");

                if (alphaU.type() == mp::PT_NUMBER) // <float></float>
                {
                    bsdf.possibleData.alpha.x() = alphaU.getNumber();
                }
                else
                {
                    bsdf.possibleData.alpha.x() = 1.0;
                }
            }
            if (props.contains("alpha_v"))
            {
                auto alphaV = props.at("alpha_v");

                if (alphaV.type() == mp::PT_NUMBER) // <float></float>
                {
                    bsdf.possibleData.alpha.y() = alphaV.getNumber();
                }
                else
                {
                    bsdf.possibleData.alpha.y() = 1.0;
                }
            }

            // alpha, alpha_u, alpha_v
            if (props.contains("alpha"))
            {
                auto alpha = props.at("alpha");

                if (alpha.type() == mp::PT_NUMBER) // <float></float>
                {
                    bsdf.possibleData.alpha = vec3(alpha.getNumber(), alpha.getNumber(), 0.0);
                }
                else
                {
                    bsdf.possibleData.alpha = vec3(1.0, 1.0, 0.0);
                }
            }
            if (props.contains("alpha_u"))
            {
                auto alphaU = props.at("alpha_u");

                if (alphaU.type() == mp::PT_NUMBER) // <float></float>
                {
                    bsdf.possibleData.alpha.x() = alphaU.getNumber();
                }
                else
                {
                    bsdf.possibleData.alpha.x() = 1.0;
                }
            }
            if (props.contains("alpha_v"))
            {
                auto alphaV = props.at("alpha_v");

                if (alphaV.type() == mp::PT_NUMBER) // <float></float>
                {
                    bsdf.possibleData.alpha.y() = alphaV.getNumber();
                }
                else
                {
                    bsdf.possibleData.alpha.y() = 1.0;
                }
            }
            for (const auto& textureChild : bsdfObject->namedChildren())
            {
                // texture
                if (textureChild.first != "alpha" || textureChild.second->type() != mp::OT_TEXTURE)
                {
                    continue;
                }
                for (const auto& textureProperty : textureChild.second->properties())
                {
                    if (textureProperty.first == "filename")
                    {
                        bsdf.possibleData.alphaTexture = imagesToLoad.size();
                        imagesToLoad.push_back(textureProperty.second.getString());
                    }
                }
            }
        }
        break;
    }
    case EBxDFType::smoothPlastic:
    case EBxDFType::roughPlastic:
    {
        if (props.contains("int_ior"))
        {
            auto interiorIor = props.at("int_ior");

            if (interiorIor.type() == mp::PT_NUMBER) // <float></float>
            {
                bsdf.possibleData.interiorIor = interiorIor.getNumber();
            }
            if (interiorIor.type() == mp::PT_STRING) // <string></string>
            {
                bsdf.possibleData.interiorIor = iorFromString(interiorIor.getString());
            }
        }
        if (props.contains("ext_ior"))
        {
            auto exteriorIor = props.at("ext_ior");

            if (exteriorIor.type() == mp::PT_NUMBER) // <float></float>
            {
                bsdf.possibleData.exteriorIor = exteriorIor.getNumber();
            }
            if (exteriorIor.type() == mp::PT_STRING) // <string></string>
            {
                bsdf.possibleData.exteriorIor = iorFromString(exteriorIor.getString());
            }
        }
        if (props.contains("diffuse_reflectance"))
        {
            auto diffuseReflectance = props.at("diffuse_reflectance");

            if (diffuseReflectance.type() == mp::PT_COLOR) // <rgb></rgb>
            {
                auto& c                              = diffuseReflectance.getColor();
                bsdf.possibleData.diffuseReflectance = vec3(c.r, c.g, c.b);
            }
            if (diffuseReflectance.type() == mp::PT_SPECTRUM) // <spectrum></spectrum>
            {
                auto& spec = diffuseReflectance.getSpectrum();

                LinearInterpolatedSpectrum spectrum = convertMitsubaSpectrumToLIS(spec);

                bsdf.possibleData.diffuseReflectance = spectrumToRGB(spectrum);
            }
        }
        if (props.contains("specular_reflectance"))
        {
            auto specularReflectance = props.at("specular_reflectance");

            if (specularReflectance.type() == mp::PT_COLOR) // <rgb></rgb>
            {
                auto& c                               = specularReflectance.getColor();
                bsdf.possibleData.specularReflectance = vec3(c.r, c.g, c.b);
            }
            if (specularReflectance.type() == mp::PT_SPECTRUM) // <spectrum></spectrum>
            {
                auto& spec = specularReflectance.getSpectrum();

                LinearInterpolatedSpectrum spectrum = convertMitsubaSpectrumToLIS(spec);

                bsdf.possibleData.specularReflectance = spectrumToRGB(spectrum);
            }
        }
        for (const auto& textureChild : bsdfObject->namedChildren())
        {
            // texture
            if (textureChild.second->type() != mp::OT_TEXTURE)
            {
                continue;
            }
            if (textureChild.first == "diffuse_reflectance")
            {
                for (const auto& textureProperty : textureChild.second->properties())
                {
                    if (textureProperty.first == "filename")
                    {
                        bsdf.possibleData.diffuseReflectanceTexture = imagesToLoad.size();
                        imagesToLoad.push_back(textureProperty.second.getString());
                    }
                }
            }
            if (textureChild.first == "specular_reflectance")
            {
                for (const auto& textureProperty : textureChild.second->properties())
                {
                    if (textureProperty.first == "filename")
                    {
                        bsdf.possibleData.specularReflectanceTexture = imagesToLoad.size();
                        imagesToLoad.push_back(textureProperty.second.getString());
                    }
                }
            }
        }

        if (bsdf.type == EBxDFType::roughPlastic)
        {
            // alpha, alpha_u, alpha_v
            if (props.contains("alpha"))
            {
                auto alpha = props.at("alpha");

                if (alpha.type() == mp::PT_NUMBER) // <float></float>
                {
                    bsdf.possibleData.alpha = vec3(alpha.getNumber(), alpha.getNumber(), 0.0);
                }
                else
                {
                    bsdf.possibleData.alpha = vec3(1.0, 1.0, 0.0);
                }
            }
            if (props.contains("alpha_u"))
            {
                auto alphaU = props.at("alpha_u");

                if (alphaU.type() == mp::PT_NUMBER) // <float></float>
                {
                    bsdf.possibleData.alpha.x() = alphaU.getNumber();
                }
                else
                {
                    bsdf.possibleData.alpha.x() = 1.0;
                }
            }
            if (props.contains("alpha_v"))
            {
                auto alphaV = props.at("alpha_v");

                if (alphaV.type() == mp::PT_NUMBER) // <float></float>
                {
                    bsdf.possibleData.alpha.y() = alphaV.getNumber();
                }
                else
                {
                    bsdf.possibleData.alpha.y() = 1.0;
                }
            }

            // alpha, alpha_u, alpha_v
            if (props.contains("alpha"))
            {
                auto alpha = props.at("alpha");

                if (alpha.type() == mp::PT_NUMBER) // <float></float>
                {
                    bsdf.possibleData.alpha = vec3(alpha.getNumber(), alpha.getNumber(), 0.0);
                }
                else
                {
                    bsdf.possibleData.alpha = vec3(1.0, 1.0, 0.0);
                }
            }
            if (props.contains("alpha_u"))
            {
                auto alphaU = props.at("alpha_u");

                if (alphaU.type() == mp::PT_NUMBER) // <float></float>
                {
                    bsdf.possibleData.alpha.x() = alphaU.getNumber();
                }
                else
                {
                    bsdf.possibleData.alpha.x() = 1.0;
                }
            }
            if (props.contains("alpha_v"))
            {
                auto alphaV = props.at("alpha_v");

                if (alphaV.type() == mp::PT_NUMBER) // <float></float>
                {
                    bsdf.possibleData.alpha.y() = alphaV.getNumber();
                }
                else
                {
                    bsdf.possibleData.alpha.y() = 1.0;
                }
            }
            for (const auto& textureChild : bsdfObject->namedChildren())
            {
                // texture
                if (textureChild.first != "alpha" || textureChild.second->type() != mp::OT_TEXTURE)
                {
                    continue;
                }
                for (const auto& textureProperty : textureChild.second->properties())
                {
                    if (textureProperty.first == "filename")
                    {
                        bsdf.possibleData.alphaTexture = imagesToLoad.size();
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
                auto& spec = radiance.getSpectrum();

                LinearInterpolatedSpectrum spectrum = convertMitsubaSpectrumToLIS(spec);

                emitter.possibleData.radiance = spectrumToRGB(spectrum);
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
    std::map<str, MitsubaBSDF> mitsubaBSDFs;
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
            if (pluginType == "sphere")
            {
                shape.type = EShapeType::sphere;
            }
            if (pluginType == "rectangle")
            {
                shape.type     = EShapeType::rectangle;
                shape.filename = "assets\\internal\\rectangle.ply";
            }
            if (pluginType == "cube")
            {
                shape.type     = EShapeType::cube;
                shape.filename = "assets\\internal\\cube.obj";
            }

            shape.transformationMatrix = mat4::Identity();
            for (const auto& prop : object->properties())
            {
                if (pluginType == "ply" && prop.first == "filename")
                {
                    shape.type     = EShapeType::triangleMesh;
                    shape.filename = fs::path(filename).parent_path().concat("\\" + prop.second.getString()).string();
                }
                if (pluginType == "obj" && prop.first == "filename")
                {
                    shape.type     = EShapeType::triangleMesh;
                    shape.filename = fs::path(filename).parent_path().concat("\\" + prop.second.getString()).string();
                }

                if (pluginType == "sphere")
                {
                    if (prop.first == "center")
                    {
                        const auto& sCenter                          = prop.second.getVector();
                        shape.transformationMatrix.block<3, 1>(0, 3) = vec3(sCenter.x, sCenter.y, sCenter.z);
                        RAYCE_LOG_INFO("------------------------- %f, %f, %f", sCenter.x, sCenter.y, sCenter.z);
                    }
                    if (prop.first == "radius")
                    {
                        const auto& sRadius                                     = prop.second.getNumber();
                        shape.transformationMatrix.block<3, 3>(0, 0).diagonal() = vec3(sRadius, sRadius, sRadius);
                    }
                }

                if (prop.first == "to_world")
                {
                    const auto& transform = prop.second.getTransform();
                    for (int i = 0; i < 4; i++)
                    {
                        for (int j = 0; j < 4; j++)
                        {
                            shape.transformationMatrix(i, j) = transform(i, j);
                        }
                    }
                }

                if (prop.first == "face_normals")
                {
                    shape.faceNormals = prop.second.getBool();
                }
            }

            for (const auto& child : object->anonymousChildren())
            {
                if (child->type() == mp::OT_BSDF)
                {
                    if (mitsubaBSDFs.find(child->id()) != mitsubaBSDFs.end())
                    {
                        shape.bsdf = child->id();
                        continue;
                    }
                    MitsubaBSDF mbsdf = loadMitsubaBSDF(child, imagesToLoad);

                    if (mbsdf.id.empty())
                    {
                        mbsdf.id = "inline_bsdf_" + std::to_string(inlineBSDFId++); // FIXME: potential collisions
                    }

                    shape.bsdf               = mbsdf.id;
                    mitsubaBSDFs[shape.bsdf] = mbsdf;
                    continue;
                }

                if (child->type() == mp::OT_EMITTER)
                {
                    MitsubaEmitter emitter = loadMitsubaEmitter(child, imagesToLoad);
                    shape.emitter          = mitsubaEmitters.size();
                    mitsubaEmitters.push_back(emitter);
                    continue;
                }
            }
            mitsubaShapes.push_back(shape);
            break;
        }
        case mp::OT_BSDF:
        {
            if (mitsubaBSDFs.find(object->id()) != mitsubaBSDFs.end())
            {
                break;
            }
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

    mImages.resize(imagesToLoad.size());
    mImageViews.resize(imagesToLoad.size());
    mImageSamplers.resize(imagesToLoad.size());

    for (auto& [ref, bsdf] : mitsubaBSDFs)
    {
        RAYCE_LOG_INFO("Creating material from %s.", ref.c_str());
        RAYCE_LOG_INFO("BSDF Type: %d", bsdf.type);

        switch (bsdf.type)
        {
        case EBxDFType::lambertDiffuse:
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
                uint32 components = STBI_rgb_alpha;
                uint32 imageSize  = width * height * components;

                VkFormat format = getImageFormat(components, false);

                VkExtent2D extent{ width, height };
                mImages[bsdf.possibleData.diffuseReflectanceTexture] = (std::make_unique<Image>(logicalDevice, extent, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
                auto& addedImage                                     = mImages[bsdf.possibleData.diffuseReflectanceTexture];
                addedImage->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                VkExtent3D extent3D{ width, height, 1 };
                Image::uploadImageDataWithStagingBuffer(logicalDevice, commandPool, *addedImage, mImageCache[name], imageSize, extent3D);
                addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                mImageViews[bsdf.possibleData.diffuseReflectanceTexture]    = (std::make_unique<ImageView>(logicalDevice, *addedImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
                mImageSamplers[bsdf.possibleData.diffuseReflectanceTexture] = (std::make_unique<Sampler>(logicalDevice, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                                                                                                         VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_MIPMAP_MODE_LINEAR, true, false, VK_COMPARE_OP_ALWAYS)); // default sampler
            }
            break;
        }
        case EBxDFType::smoothDielectric:
        case EBxDFType::smoothDielectricThin:
        case EBxDFType::roughDielectric:
        {
            if (bsdf.possibleData.specularReflectanceTexture >= 0)
            {
                // load image;
                str name = bsdf.id + "_specularReflectance";

                if (mImageCache[name])
                {
                    continue;
                }

                str imageFile = imagesToLoad[bsdf.possibleData.specularReflectanceTexture];

                int32 w, h, c;

                if (!fs::exists(imageFile))
                {
                    imageFile = fs::path(filename).parent_path().concat("\\" + imageFile).string();
                    if (!fs::exists(imageFile))
                    {
                        RAYCE_LOG_ERROR("Can not find %s nor %s", imagesToLoad[bsdf.possibleData.specularReflectanceTexture].c_str(), imageFile.c_str());
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
                uint32 components = STBI_rgb_alpha;
                uint32 imageSize  = width * height * components;

                VkFormat format = getImageFormat(components, false);

                VkExtent2D extent{ width, height };
                mImages[bsdf.possibleData.specularReflectanceTexture] = (std::make_unique<Image>(logicalDevice, extent, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
                auto& addedImage                                      = mImages[bsdf.possibleData.specularReflectanceTexture];
                addedImage->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                VkExtent3D extent3D{ width, height, 1 };
                Image::uploadImageDataWithStagingBuffer(logicalDevice, commandPool, *addedImage, mImageCache[name], imageSize, extent3D);
                addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                mImageViews[bsdf.possibleData.specularReflectanceTexture]    = (std::make_unique<ImageView>(logicalDevice, *addedImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
                mImageSamplers[bsdf.possibleData.specularReflectanceTexture] = (std::make_unique<Sampler>(logicalDevice, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                                                                                                          VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_MIPMAP_MODE_LINEAR, true, false, VK_COMPARE_OP_ALWAYS)); // default sampler
            }
            if (bsdf.possibleData.specularTransmittanceTexture >= 0)
            {
                // load image;
                str name = bsdf.id + "_specularTransmittance";

                if (mImageCache[name])
                {
                    continue;
                }

                str imageFile = imagesToLoad[bsdf.possibleData.specularTransmittanceTexture];

                int32 w, h, c;

                if (!fs::exists(imageFile))
                {
                    imageFile = fs::path(filename).parent_path().concat("\\" + imageFile).string();
                    if (!fs::exists(imageFile))
                    {
                        RAYCE_LOG_ERROR("Can not find %s nor %s", imagesToLoad[bsdf.possibleData.specularTransmittanceTexture].c_str(), imageFile.c_str());
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
                uint32 components = STBI_rgb_alpha;
                uint32 imageSize  = width * height * components;

                VkFormat format = getImageFormat(components, false);

                VkExtent2D extent{ width, height };
                mImages[bsdf.possibleData.specularTransmittanceTexture] = (std::make_unique<Image>(logicalDevice, extent, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
                auto& addedImage                                        = mImages[bsdf.possibleData.specularTransmittanceTexture];
                addedImage->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                VkExtent3D extent3D{ width, height, 1 };
                Image::uploadImageDataWithStagingBuffer(logicalDevice, commandPool, *addedImage, mImageCache[name], imageSize, extent3D);
                addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                mImageViews[bsdf.possibleData.specularTransmittanceTexture]    = (std::make_unique<ImageView>(logicalDevice, *addedImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
                mImageSamplers[bsdf.possibleData.specularTransmittanceTexture] = (std::make_unique<Sampler>(logicalDevice, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                                                                                                            VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_MIPMAP_MODE_LINEAR, true, false, VK_COMPARE_OP_ALWAYS)); // default sampler
            }

            if (bsdf.type == EBxDFType::roughDielectric)
            {
                if (bsdf.possibleData.alphaTexture >= 0)
                {
                    // load image;
                    str name = bsdf.id + "_alpha";

                    if (mImageCache[name])
                    {
                        continue;
                    }

                    str imageFile = imagesToLoad[bsdf.possibleData.alphaTexture];

                    int32 w, h, c;

                    if (!fs::exists(imageFile))
                    {
                        imageFile = fs::path(filename).parent_path().concat("\\" + imageFile).string();
                        if (!fs::exists(imageFile))
                        {
                            RAYCE_LOG_ERROR("Can not find %s nor %s", imagesToLoad[bsdf.possibleData.alphaTexture].c_str(), imageFile.c_str());
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
                    uint32 components = STBI_rgb_alpha;
                    uint32 imageSize  = width * height * components;

                    VkFormat format = getImageFormat(components, false);

                    VkExtent2D extent{ width, height };
                    mImages[bsdf.possibleData.alphaTexture] = (std::make_unique<Image>(logicalDevice, extent, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
                    auto& addedImage                        = mImages[bsdf.possibleData.alphaTexture];
                    addedImage->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                    addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                    VkExtent3D extent3D{ width, height, 1 };
                    Image::uploadImageDataWithStagingBuffer(logicalDevice, commandPool, *addedImage, mImageCache[name], imageSize, extent3D);
                    addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                    mImageViews[bsdf.possibleData.alphaTexture]    = (std::make_unique<ImageView>(logicalDevice, *addedImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
                    mImageSamplers[bsdf.possibleData.alphaTexture] = (std::make_unique<Sampler>(logicalDevice, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                                                                                                VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_MIPMAP_MODE_LINEAR, true, false, VK_COMPARE_OP_ALWAYS)); // default sampler
                }
            }
            break;
        }
        case EBxDFType::smoothConductor:
        case EBxDFType::roughConductor:
        {

            if (bsdf.possibleData.specularReflectanceTexture >= 0)
            {
                // load image;
                str name = bsdf.id + "_specularReflectance";

                if (mImageCache[name])
                {
                    continue;
                }

                str imageFile = imagesToLoad[bsdf.possibleData.specularReflectanceTexture];

                int32 w, h, c;

                if (!fs::exists(imageFile))
                {
                    imageFile = fs::path(filename).parent_path().concat("\\" + imageFile).string();
                    if (!fs::exists(imageFile))
                    {
                        RAYCE_LOG_ERROR("Can not find %s nor %s", imagesToLoad[bsdf.possibleData.specularReflectanceTexture].c_str(), imageFile.c_str());
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
                uint32 components = STBI_rgb_alpha;
                uint32 imageSize  = width * height * components;

                VkFormat format = getImageFormat(components, false);

                VkExtent2D extent{ width, height };
                mImages[bsdf.possibleData.specularReflectanceTexture] = (std::make_unique<Image>(logicalDevice, extent, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
                auto& addedImage                                      = mImages[bsdf.possibleData.specularReflectanceTexture];
                addedImage->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                VkExtent3D extent3D{ width, height, 1 };
                Image::uploadImageDataWithStagingBuffer(logicalDevice, commandPool, *addedImage, mImageCache[name], imageSize, extent3D);
                addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                mImageViews[bsdf.possibleData.specularReflectanceTexture]    = (std::make_unique<ImageView>(logicalDevice, *addedImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
                mImageSamplers[bsdf.possibleData.specularReflectanceTexture] = (std::make_unique<Sampler>(logicalDevice, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                                                                                                          VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_MIPMAP_MODE_LINEAR, true, false, VK_COMPARE_OP_ALWAYS)); // default sampler
            }
            if (bsdf.possibleData.conductorEtaTexture >= 0)
            {
                // load image;
                str name = bsdf.id + "_conductorEtaTexture";

                if (mImageCache[name])
                {
                    continue;
                }

                str imageFile = imagesToLoad[bsdf.possibleData.conductorEtaTexture];

                int32 w, h, c;

                if (!fs::exists(imageFile))
                {
                    imageFile = fs::path(filename).parent_path().concat("\\" + imageFile).string();
                    if (!fs::exists(imageFile))
                    {
                        RAYCE_LOG_ERROR("Can not find %s nor %s", imagesToLoad[bsdf.possibleData.conductorEtaTexture].c_str(), imageFile.c_str());
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
                uint32 components = STBI_rgb_alpha;
                uint32 imageSize  = width * height * components;

                VkFormat format = getImageFormat(components, false);

                VkExtent2D extent{ width, height };
                mImages[bsdf.possibleData.conductorEtaTexture] = (std::make_unique<Image>(logicalDevice, extent, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
                auto& addedImage                               = mImages[bsdf.possibleData.conductorEtaTexture];
                addedImage->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                VkExtent3D extent3D{ width, height, 1 };
                Image::uploadImageDataWithStagingBuffer(logicalDevice, commandPool, *addedImage, mImageCache[name], imageSize, extent3D);
                addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                mImageViews[bsdf.possibleData.conductorEtaTexture]    = (std::make_unique<ImageView>(logicalDevice, *addedImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
                mImageSamplers[bsdf.possibleData.conductorEtaTexture] = (std::make_unique<Sampler>(logicalDevice, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                                                                                                   VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_MIPMAP_MODE_LINEAR, true, false, VK_COMPARE_OP_ALWAYS)); // default sampler
            }
            if (bsdf.possibleData.conductorKTexture >= 0)
            {
                // load image;
                str name = bsdf.id + "_conductorKTexture";

                if (mImageCache[name])
                {
                    continue;
                }

                str imageFile = imagesToLoad[bsdf.possibleData.conductorKTexture];

                int32 w, h, c;

                if (!fs::exists(imageFile))
                {
                    imageFile = fs::path(filename).parent_path().concat("\\" + imageFile).string();
                    if (!fs::exists(imageFile))
                    {
                        RAYCE_LOG_ERROR("Can not find %s nor %s", imagesToLoad[bsdf.possibleData.conductorKTexture].c_str(), imageFile.c_str());
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
                uint32 components = STBI_rgb_alpha;
                uint32 imageSize  = width * height * components;

                VkFormat format = getImageFormat(components, false);

                VkExtent2D extent{ width, height };
                mImages[bsdf.possibleData.conductorKTexture] = (std::make_unique<Image>(logicalDevice, extent, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
                auto& addedImage                             = mImages[bsdf.possibleData.conductorKTexture];
                addedImage->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                VkExtent3D extent3D{ width, height, 1 };
                Image::uploadImageDataWithStagingBuffer(logicalDevice, commandPool, *addedImage, mImageCache[name], imageSize, extent3D);
                addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                mImageViews[bsdf.possibleData.conductorKTexture]    = (std::make_unique<ImageView>(logicalDevice, *addedImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
                mImageSamplers[bsdf.possibleData.conductorKTexture] = (std::make_unique<Sampler>(logicalDevice, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                                                                                                 VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_MIPMAP_MODE_LINEAR, true, false, VK_COMPARE_OP_ALWAYS)); // default sampler
            }

            if (bsdf.type == EBxDFType::roughConductor)
            {
                if (bsdf.possibleData.alphaTexture >= 0)
                {
                    // load image;
                    str name = bsdf.id + "_alpha";

                    if (mImageCache[name])
                    {
                        continue;
                    }

                    str imageFile = imagesToLoad[bsdf.possibleData.alphaTexture];

                    int32 w, h, c;

                    if (!fs::exists(imageFile))
                    {
                        imageFile = fs::path(filename).parent_path().concat("\\" + imageFile).string();
                        if (!fs::exists(imageFile))
                        {
                            RAYCE_LOG_ERROR("Can not find %s nor %s", imagesToLoad[bsdf.possibleData.alphaTexture].c_str(), imageFile.c_str());
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
                    uint32 components = STBI_rgb_alpha;
                    uint32 imageSize  = width * height * components;

                    VkFormat format = getImageFormat(components, false);

                    VkExtent2D extent{ width, height };
                    mImages[bsdf.possibleData.alphaTexture] = (std::make_unique<Image>(logicalDevice, extent, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
                    auto& addedImage                        = mImages[bsdf.possibleData.alphaTexture];
                    addedImage->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                    addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                    VkExtent3D extent3D{ width, height, 1 };
                    Image::uploadImageDataWithStagingBuffer(logicalDevice, commandPool, *addedImage, mImageCache[name], imageSize, extent3D);
                    addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                    mImageViews[bsdf.possibleData.alphaTexture]    = (std::make_unique<ImageView>(logicalDevice, *addedImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
                    mImageSamplers[bsdf.possibleData.alphaTexture] = (std::make_unique<Sampler>(logicalDevice, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                                                                                                VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_MIPMAP_MODE_LINEAR, true, false, VK_COMPARE_OP_ALWAYS)); // default sampler
                }
            }
            break;
        }
        case EBxDFType::smoothPlastic:
        case EBxDFType::roughPlastic:
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
                uint32 components = STBI_rgb_alpha;
                uint32 imageSize  = width * height * components;

                VkFormat format = getImageFormat(components, false);

                VkExtent2D extent{ width, height };
                mImages[bsdf.possibleData.diffuseReflectanceTexture] = (std::make_unique<Image>(logicalDevice, extent, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
                auto& addedImage                                     = mImages[bsdf.possibleData.diffuseReflectanceTexture];
                addedImage->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                VkExtent3D extent3D{ width, height, 1 };
                Image::uploadImageDataWithStagingBuffer(logicalDevice, commandPool, *addedImage, mImageCache[name], imageSize, extent3D);
                addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                mImageViews[bsdf.possibleData.diffuseReflectanceTexture]    = (std::make_unique<ImageView>(logicalDevice, *addedImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
                mImageSamplers[bsdf.possibleData.diffuseReflectanceTexture] = (std::make_unique<Sampler>(logicalDevice, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                                                                                                         VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_MIPMAP_MODE_LINEAR, true, false, VK_COMPARE_OP_ALWAYS)); // default sampler
            }
            if (bsdf.possibleData.specularReflectanceTexture >= 0)
            {
                // load image;
                str name = bsdf.id + "_specularReflectance";

                if (mImageCache[name])
                {
                    continue;
                }

                str imageFile = imagesToLoad[bsdf.possibleData.specularReflectanceTexture];

                int32 w, h, c;

                if (!fs::exists(imageFile))
                {
                    imageFile = fs::path(filename).parent_path().concat("\\" + imageFile).string();
                    if (!fs::exists(imageFile))
                    {
                        RAYCE_LOG_ERROR("Can not find %s nor %s", imagesToLoad[bsdf.possibleData.specularReflectanceTexture].c_str(), imageFile.c_str());
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
                uint32 components = STBI_rgb_alpha;
                uint32 imageSize  = width * height * components;

                VkFormat format = getImageFormat(components, false);

                VkExtent2D extent{ width, height };
                mImages[bsdf.possibleData.specularReflectanceTexture] = (std::make_unique<Image>(logicalDevice, extent, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
                auto& addedImage                                      = mImages[bsdf.possibleData.specularReflectanceTexture];
                addedImage->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                VkExtent3D extent3D{ width, height, 1 };
                Image::uploadImageDataWithStagingBuffer(logicalDevice, commandPool, *addedImage, mImageCache[name], imageSize, extent3D);
                addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                mImageViews[bsdf.possibleData.specularReflectanceTexture]    = (std::make_unique<ImageView>(logicalDevice, *addedImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
                mImageSamplers[bsdf.possibleData.specularReflectanceTexture] = (std::make_unique<Sampler>(logicalDevice, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                                                                                                          VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_MIPMAP_MODE_LINEAR, true, false, VK_COMPARE_OP_ALWAYS)); // default sampler
            }

            if (bsdf.type == EBxDFType::roughPlastic)
            {
                if (bsdf.possibleData.alphaTexture >= 0)
                {
                    // load image;
                    str name = bsdf.id + "_alpha";

                    if (mImageCache[name])
                    {
                        continue;
                    }

                    str imageFile = imagesToLoad[bsdf.possibleData.alphaTexture];

                    int32 w, h, c;

                    if (!fs::exists(imageFile))
                    {
                        imageFile = fs::path(filename).parent_path().concat("\\" + imageFile).string();
                        if (!fs::exists(imageFile))
                        {
                            RAYCE_LOG_ERROR("Can not find %s nor %s", imagesToLoad[bsdf.possibleData.alphaTexture].c_str(), imageFile.c_str());
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
                    uint32 components = STBI_rgb_alpha;
                    uint32 imageSize  = width * height * components;

                    VkFormat format = getImageFormat(components, false);

                    VkExtent2D extent{ width, height };
                    mImages[bsdf.possibleData.alphaTexture] = (std::make_unique<Image>(logicalDevice, extent, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
                    auto& addedImage                        = mImages[bsdf.possibleData.alphaTexture];
                    addedImage->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                    addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                    VkExtent3D extent3D{ width, height, 1 };
                    Image::uploadImageDataWithStagingBuffer(logicalDevice, commandPool, *addedImage, mImageCache[name], imageSize, extent3D);
                    addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                    mImageViews[bsdf.possibleData.alphaTexture]    = (std::make_unique<ImageView>(logicalDevice, *addedImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
                    mImageSamplers[bsdf.possibleData.alphaTexture] = (std::make_unique<Sampler>(logicalDevice, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                                                                                                VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_MIPMAP_MODE_LINEAR, true, false, VK_COMPARE_OP_ALWAYS)); // default sampler
                }
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
    mImageSamplers.push_back(std::make_unique<Sampler>(logicalDevice, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                                                       VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_MIPMAP_MODE_LINEAR, true, false, VK_COMPARE_OP_ALWAYS)); // default sampler

    // emitters -> lights
    int32 emitterId = 0;
    for (auto& emitter : mitsubaEmitters)
    {
        RAYCE_LOG_INFO("Creating light from emitter %d.", emitterId);
        emitter.possibleData.type = emitter.type;

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
                uint32 components = STBI_rgb_alpha;
                uint32 imageSize  = width * height * components;

                VkFormat format = getImageFormat(components, false);

                VkExtent2D extent{ width, height };
                mImages[emitter.possibleData.radianceTexture] = (std::make_unique<Image>(logicalDevice, extent, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
                auto& addedImage                              = mImages[emitter.possibleData.radianceTexture];
                addedImage->allocateMemory(logicalDevice, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                VkExtent3D extent3D{ width, height, 1 };
                Image::uploadImageDataWithStagingBuffer(logicalDevice, commandPool, *addedImage, mImageCache[name], imageSize, extent3D);
                addedImage->adaptImageLayout(logicalDevice, commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                mImageViews[emitter.possibleData.radianceTexture]    = (std::make_unique<ImageView>(logicalDevice, *addedImage, format, VK_IMAGE_ASPECT_COLOR_BIT));
                mImageSamplers[emitter.possibleData.radianceTexture] = (std::make_unique<Sampler>(logicalDevice, VK_FILTER_LINEAR, VK_FILTER_LINEAR,
                                                                                                  VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_MIPMAP_MODE_LINEAR, true, false, VK_COMPARE_OP_ALWAYS)); // default sampler
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
    uint32 meshId   = 0;
    uint32 sphereId = 0;
    for (auto& shape : mitsubaShapes)
    {
        if (shape.type == EShapeType::triangleMesh || shape.type == EShapeType::rectangle || shape.type == EShapeType::cube)
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
                mReflectionInfo.meshTriCounts[meshId + sphereId] += primitiveCount;

                // materialId is filled before
                uint32 materialId = mitsubaBSDFs[shape.bsdf].materialId;
                int32 lightId     = -1;
                if (shape.emitter >= 0 && shape.type == EShapeType::rectangle)
                {
                    lightId = mitsubaEmitters[shape.emitter].lightId;

                    // convert light data to our type
                    std::unique_ptr<Light>& lightData = mLights[lightId];

                    assert(lightData->type == ELightType::area); // atm analytic rectangle

                    if (shape.type == EShapeType::rectangle)
                    {
                        lightData->type = ELightType::analyticRectangle;

                        lightData->wCenter      = (shape.transformationMatrix * vec4(0.0, 0.0, 0.0, 1.0)).head<3>(); // rectangle is xy [-1, 1]
                        vec3 tmp                = (shape.transformationMatrix.block<3, 3>(0, 0) * vec3(2.0, 2.0, 0.0));
                        lightData->surfaceArea  = std::abs(tmp.squaredNorm());
                        lightData->lightToWorld = shape.transformationMatrix;
                        lightData->worldToLight = shape.transformationMatrix.inverse();
                    }
                }
                mMaterials[materialId]->canUseUv = hasUVs;
                mMaterials[materialId]->faceNormals = shape.faceNormals;
                pGeometry->add(std::move(vertexBuffer), maxVertex, std::move(indexBuffer), primitiveCount, materialId, lightId, { shape.transformationMatrix });
            }

            if (ext == "obj")
            {
                RAYCE_LOG_INFO("Loading: %s.", shape.filename.c_str());
                tinyobj::ObjReader objReader;
                tinyobj::ObjReaderConfig config;
                config.triangulate = true;
                bool success       = objReader.ParseFromFile(shape.filename.c_str(), config);

                if (!success)
                {
                    RAYCE_LOG_ERROR("Can not load: %s!", shape.filename.c_str());
                    continue;
                }

                if (!objReader.Warning().empty())
                {
                    RAYCE_LOG_WARN("TinyObjReader: %s", objReader.Warning().c_str());
                }

                auto& attrib = objReader.GetAttrib();
                auto& shapes = objReader.GetShapes();

                // FIXME: We could support setting in mitsuba files...
                bool faceNormals = shape.type == EShapeType::cube; // ignore smoothing groups

                bool hasPositions = !attrib.vertices.empty(), hasIndices = hasPositions, hasNormals = !attrib.normals.empty(), hasUVs = !attrib.texcoords.empty();
                std::vector<uint32> indices;
                std::vector<vec3> positions;
                std::vector<vec3> normals;
                std::vector<vec2> uvs;
                uint32 vertIdx = 0;

                std::unordered_map<uint32, uint32> simpleCache;

                for (size_t s = 0; s < shapes.size(); ++s)
                {
                    size_t indexOffset = 0;
                    for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); ++f)
                    {
                        size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
                        positions.resize(positions.size() + fv);
                        normals.resize(positions.size() + fv);
                        uvs.resize(positions.size() + fv);

                        for (size_t v = 0; v < fv; v++)
                        {
                            tinyobj::index_t idx = shapes[s].mesh.indices[indexOffset + v];

                            if (!faceNormals && simpleCache.find(idx.vertex_index) != simpleCache.end())
                            {
                                uint32 vIdx = simpleCache[idx.vertex_index];
                                indices.push_back(vIdx);
                                continue;
                            }

                            indices.push_back(vertIdx);

                            simpleCache[idx.vertex_index] = vertIdx;

                            tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                            tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                            tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

                            positions[vertIdx] = vec3(vx, vy, vz);

                            if (idx.normal_index >= 0)
                            {
                                tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                                tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                                tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];

                                normals[vertIdx] = vec3(nx, ny, nz);
                            }

                            if (idx.texcoord_index >= 0)
                            {
                                tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                                tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];

                                uvs[vertIdx] = vec2(tx, ty);
                            }
                            vertIdx++;
                        }
                        indexOffset += fv;
                    }
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
                mReflectionInfo.meshTriCounts[meshId + sphereId] += primitiveCount;

                // materialId is filled before
                uint32 materialId = mitsubaBSDFs[shape.bsdf].materialId;
                int32 lightId     = -1;
                if (shape.emitter >= 0 && shape.type == EShapeType::rectangle)
                {
                    lightId = mitsubaEmitters[shape.emitter].lightId;

                    // convert light data to our type
                    std::unique_ptr<Light>& lightData = mLights[lightId];

                    assert(lightData->type == ELightType::area); // atm analytic rectangle

                    if (shape.type == EShapeType::rectangle)
                    {
                        lightData->type = ELightType::analyticRectangle;

                        lightData->wCenter      = (shape.transformationMatrix * vec4(0.0, 0.0, 0.0, 1.0)).head<3>(); // rectangle is xy [-1, 1]
                        vec3 tmp                = (shape.transformationMatrix.block<3, 3>(0, 0) * vec3(2.0, 2.0, 0.0));
                        lightData->surfaceArea  = std::abs(tmp.squaredNorm());
                        lightData->lightToWorld = shape.transformationMatrix;
                        lightData->worldToLight = shape.transformationMatrix.inverse();
                    }
                }
                mMaterials[materialId]->canUseUv = hasUVs;
                mMaterials[materialId]->faceNormals = shape.faceNormals;
                pGeometry->add(std::move(vertexBuffer), maxVertex, std::move(indexBuffer), primitiveCount, materialId, lightId, { shape.transformationMatrix });
            }

            meshId++;
        }
        else if (shape.type == EShapeType::sphere)
        {
            std::unique_ptr<Sphere> sphere                      = std::make_unique<Sphere>();
            sphere->center                                      = (shape.transformationMatrix * vec4(0.0, 0.0, 0.0, 1.0)).head<3>();
            sphere->radius                                      = ((shape.transformationMatrix * vec4(1.0, 0.0, 0.0, 1.0)).head<3>() - sphere->center).norm();
            std::unique_ptr<AxisAlignedBoundingBox> boundingBox = std::make_unique<AxisAlignedBoundingBox>();
            boundingBox->minimum                                = sphere->center - vec3(sphere->radius, sphere->radius, sphere->radius);
            boundingBox->maximum                                = sphere->center + vec3(sphere->radius, sphere->radius, sphere->radius);
            // materialId is filled before
            uint32 materialId = mitsubaBSDFs[shape.bsdf].materialId;
            int32 lightId     = -1;
            if (shape.emitter >= 0)
            {
                lightId = mitsubaEmitters[shape.emitter].lightId;

                // convert light data to our type
                std::unique_ptr<Light>& lightData = mLights[lightId];

                assert(lightData->type == ELightType::area); // atm analytic sphere

                if (shape.type == EShapeType::sphere)
                {
                    lightData->type = ELightType::analyticSphere;

                    lightData->wCenter      = sphere->center;
                    lightData->surfaceArea  = (2.0 * TWO_PI * sphere->radius * sphere->radius);
                    lightData->lightToWorld = shape.transformationMatrix;
                    lightData->worldToLight = shape.transformationMatrix.inverse();
                }
            }
            mMaterials[materialId]->canUseUv = false;
            pGeometry->add(std::move(sphere), std::move(boundingBox), materialId, lightId, { mat4::Identity() });

            sphereId++;
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
