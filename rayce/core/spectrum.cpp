/// @file      spectrum.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#include "spectrum.hpp"
#include <fstream>

#include "spectrum.inl"

using namespace rayce;

const DenseSpectrum Spectra::CIEX(360.0, 830.0, CIE_X, CIESampleCount);
const DenseSpectrum Spectra::CIEY(360.0, 830.0, CIE_Y, CIESampleCount);
const DenseSpectrum Spectra::CIEZ(360.0, 830.0, CIE_Z, CIESampleCount);

LinearInterpolatedSpectrum LinearInterpolatedSpectrum::fromFile(const str& filename)
{
    // FIXME: Implement!
    std::vector<float> wavelengths;
    std::vector<float> values;

    std::ifstream input(filename);
    if (input.bad() || input.fail())
    {
        RAYCE_LOG_ERROR("Can not open spd file %s", filename.c_str());
        LinearInterpolatedSpectrum spectrum = LinearInterpolatedSpectrum(wavelengths, values);
        return spectrum;
    }

    str line;
    while (true)
    {
        if (!std::getline(input, line))
        {
            break;
        }

        line = trim(line);
        if (line.length() == 0 || line[0] == '#')
        {
            continue;
        }

        std::istringstream iss(line);
        float lambda, value;
        if (!(iss >> lambda >> value))
        {
            break;
        }

        wavelengths.push_back(lambda);
        values.push_back(value);
    }

    LinearInterpolatedSpectrum spectrum = LinearInterpolatedSpectrum(wavelengths, values);

    return spectrum;
}

LinearInterpolatedSpectrum LinearInterpolatedSpectrum::fromInterleaved(const float* interleaved, ptr_size count, bool normalize)
{
    std::vector<float> wavelengths(count);
    std::vector<float> values(count);

    for (ptr_size i = 0; i < count; ++i)
    {
        wavelengths[i] = interleaved[i * 2];
        values[i]      = interleaved[i * 2 + 1];

        assert(i == 0 || wavelengths[i] >= wavelengths[i - 1]);
    }

    LinearInterpolatedSpectrum spectrum = LinearInterpolatedSpectrum(wavelengths, values);

    if (normalize)
    {
        spectrum.scale(Spectra::CIEYIntegral / innerProduct(spectrum, Spectra::CIEY));
    }

    return spectrum;
}

static const std::unordered_map<str, LinearInterpolatedSpectrum> namedSpectra{
    { "glass-BK7", LinearInterpolatedSpectrum::fromInterleaved(GlassBK7_eta, 29, false) },
    { "glass-BAF10", LinearInterpolatedSpectrum::fromInterleaved(GlassBAF10_eta, 27, false) },
    { "glass-FK51A", LinearInterpolatedSpectrum::fromInterleaved(GlassFK51A_eta, 29, false) },
    { "glass-LASF9", LinearInterpolatedSpectrum::fromInterleaved(GlassLASF9_eta, 26, false) },
    { "glass-F5", LinearInterpolatedSpectrum::fromInterleaved(GlassSF5_eta, 26, false) },
    { "glass-F10", LinearInterpolatedSpectrum::fromInterleaved(GlassSF10_eta, 26, false) },
    { "glass-F11", LinearInterpolatedSpectrum::fromInterleaved(GlassSF11_eta, 26, false) },

    { "metal-Ag-eta", LinearInterpolatedSpectrum::fromInterleaved(Ag_eta, 56, false) },
    { "metal-Ag-k", LinearInterpolatedSpectrum::fromInterleaved(Ag_k, 56, false) },
    { "metal-Al-eta", LinearInterpolatedSpectrum::fromInterleaved(Al_eta, 56, false) },
    { "metal-Al-k", LinearInterpolatedSpectrum::fromInterleaved(Al_k, 56, false) },
    { "metal-Au-eta", LinearInterpolatedSpectrum::fromInterleaved(Au_eta, 56, false) },
    { "metal-Au-k", LinearInterpolatedSpectrum::fromInterleaved(Au_k, 56, false) },
    { "metal-Cu-eta", LinearInterpolatedSpectrum::fromInterleaved(Cu_eta, 56, false) },
    { "metal-Cu-k", LinearInterpolatedSpectrum::fromInterleaved(Cu_k, 56, false) },
    { "metal-CuZn-eta", LinearInterpolatedSpectrum::fromInterleaved(CuZn_eta, 61, false) },
    { "metal-CuZn-k", LinearInterpolatedSpectrum::fromInterleaved(CuZn_k, 61, false) },
    { "metal-MgO-eta", LinearInterpolatedSpectrum::fromInterleaved(MgO_eta, 30, false) },
    { "metal-MgO-k", LinearInterpolatedSpectrum::fromInterleaved(MgO_k, 30, false) },
    { "metal-TiO2-eta", LinearInterpolatedSpectrum::fromInterleaved(TiO2_eta, 34, false) },
    { "metal-TiO2-k", LinearInterpolatedSpectrum::fromInterleaved(TiO2_k, 34, false) },

    { "stdillum-A", LinearInterpolatedSpectrum::fromInterleaved(CIE_Illum_A, 107, true) },
    { "stdillum-D50", LinearInterpolatedSpectrum::fromInterleaved(CIE_Illum_D5000, 107, true) },
    { "stdillum-D65", LinearInterpolatedSpectrum::fromInterleaved(CIE_Illum_D6500, 107, true) },
    { "stdillum-F1", LinearInterpolatedSpectrum::fromInterleaved(CIE_Illum_F1, 81, true) },
    { "stdillum-F2", LinearInterpolatedSpectrum::fromInterleaved(CIE_Illum_F2, 81, true) },
    { "stdillum-F3", LinearInterpolatedSpectrum::fromInterleaved(CIE_Illum_F3, 81, true) },
    { "stdillum-F4", LinearInterpolatedSpectrum::fromInterleaved(CIE_Illum_F4, 81, true) },
    { "stdillum-F5", LinearInterpolatedSpectrum::fromInterleaved(CIE_Illum_F5, 81, true) },
    { "stdillum-F6", LinearInterpolatedSpectrum::fromInterleaved(CIE_Illum_F6, 81, true) },
    { "stdillum-F7", LinearInterpolatedSpectrum::fromInterleaved(CIE_Illum_F7, 81, true) },
    { "stdillum-F8", LinearInterpolatedSpectrum::fromInterleaved(CIE_Illum_F8, 81, true) },
    { "stdillum-F9", LinearInterpolatedSpectrum::fromInterleaved(CIE_Illum_F9, 81, true) },
    { "stdillum-F10", LinearInterpolatedSpectrum::fromInterleaved(CIE_Illum_F10, 81, true) },
    { "stdillum-F11", LinearInterpolatedSpectrum::fromInterleaved(CIE_Illum_F11, 81, true) },
    { "stdillum-F12", LinearInterpolatedSpectrum::fromInterleaved(CIE_Illum_F12, 81, true) },

    { "illum-acesD60", LinearInterpolatedSpectrum::fromInterleaved(ACES_Illum_D60, 107, true) },

    { "canon_eos_100d_r", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_100d_r, 35, false) },
    { "canon_eos_100d_g", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_100d_g, 35, false) },
    { "canon_eos_100d_b", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_100d_b, 35, false) },

    { "canon_eos_1dx_mkii_r", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_1dx_mkii_r, 35, false) },
    { "canon_eos_1dx_mkii_g", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_1dx_mkii_g, 35, false) },
    { "canon_eos_1dx_mkii_b", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_1dx_mkii_b, 35, false) },

    { "canon_eos_200d_r", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_200d_r, 35, false) },
    { "canon_eos_200d_g", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_200d_g, 35, false) },
    { "canon_eos_200d_b", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_200d_b, 35, false) },

    { "canon_eos_200d_mkii_r", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_200d_mkii_r, 35, false) },
    { "canon_eos_200d_mkii_g", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_200d_mkii_g, 35, false) },
    { "canon_eos_200d_mkii_b", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_200d_mkii_b, 35, false) },

    { "canon_eos_5d_r", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_5d_r, 35, false) },
    { "canon_eos_5d_g", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_5d_g, 35, false) },
    { "canon_eos_5d_b", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_5d_b, 35, false) },

    { "canon_eos_5d_mkii_r", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_5d_mkii_r, 35, false) },
    { "canon_eos_5d_mkii_g", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_5d_mkii_g, 35, false) },
    { "canon_eos_5d_mkii_b", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_5d_mkii_b, 35, false) },

    { "canon_eos_5d_mkiii_r", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_5d_mkiii_r, 35, false) },
    { "canon_eos_5d_mkiii_g", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_5d_mkiii_g, 35, false) },
    { "canon_eos_5d_mkiii_b", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_5d_mkiii_b, 35, false) },

    { "canon_eos_5d_mkiv_r", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_5d_mkiv_r, 35, false) },
    { "canon_eos_5d_mkiv_g", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_5d_mkiv_g, 35, false) },
    { "canon_eos_5d_mkiv_b", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_5d_mkiv_b, 35, false) },

    { "canon_eos_5ds_r", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_5ds_r, 35, false) },
    { "canon_eos_5ds_g", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_5ds_g, 35, false) },
    { "canon_eos_5ds_b", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_5ds_b, 35, false) },

    { "canon_eos_m_r", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_m_r, 35, false) },
    { "canon_eos_m_g", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_m_g, 35, false) },
    { "canon_eos_m_b", LinearInterpolatedSpectrum::fromInterleaved(canon_eos_m_b, 35, false) },

    { "hasselblad_l1d_20c_r", LinearInterpolatedSpectrum::fromInterleaved(hasselblad_l1d_20c_r, 35, false) },
    { "hasselblad_l1d_20c_g", LinearInterpolatedSpectrum::fromInterleaved(hasselblad_l1d_20c_g, 35, false) },
    { "hasselblad_l1d_20c_b", LinearInterpolatedSpectrum::fromInterleaved(hasselblad_l1d_20c_b, 35, false) },

    { "nikon_d810_r", LinearInterpolatedSpectrum::fromInterleaved(nikon_d810_r, 35, false) },
    { "nikon_d810_g", LinearInterpolatedSpectrum::fromInterleaved(nikon_d810_g, 35, false) },
    { "nikon_d810_b", LinearInterpolatedSpectrum::fromInterleaved(nikon_d810_b, 35, false) },

    { "nikon_d850_r", LinearInterpolatedSpectrum::fromInterleaved(nikon_d850_r, 35, false) },
    { "nikon_d850_g", LinearInterpolatedSpectrum::fromInterleaved(nikon_d850_g, 35, false) },
    { "nikon_d850_b", LinearInterpolatedSpectrum::fromInterleaved(nikon_d850_b, 35, false) },

    { "sony_ilce_6400_r", LinearInterpolatedSpectrum::fromInterleaved(sony_ilce_6400_r, 35, false) },
    { "sony_ilce_6400_g", LinearInterpolatedSpectrum::fromInterleaved(sony_ilce_6400_g, 35, false) },
    { "sony_ilce_6400_b", LinearInterpolatedSpectrum::fromInterleaved(sony_ilce_6400_b, 35, false) },

    { "sony_ilce_7m3_r", LinearInterpolatedSpectrum::fromInterleaved(sony_ilce_7m3_r, 35, false) },
    { "sony_ilce_7m3_g", LinearInterpolatedSpectrum::fromInterleaved(sony_ilce_7m3_g, 35, false) },
    { "sony_ilce_7m3_b", LinearInterpolatedSpectrum::fromInterleaved(sony_ilce_7m3_b, 35, false) },

    { "sony_ilce_7rm3_r", LinearInterpolatedSpectrum::fromInterleaved(sony_ilce_7rm3_r, 35, false) },
    { "sony_ilce_7rm3_g", LinearInterpolatedSpectrum::fromInterleaved(sony_ilce_7rm3_g, 35, false) },
    { "sony_ilce_7rm3_b", LinearInterpolatedSpectrum::fromInterleaved(sony_ilce_7rm3_b, 35, false) },

    { "sony_ilce_9_r", LinearInterpolatedSpectrum::fromInterleaved(sony_ilce_9_r, 35, false) },
    { "sony_ilce_9_g", LinearInterpolatedSpectrum::fromInterleaved(sony_ilce_9_g, 35, false) },
    { "sony_ilce_9_b", LinearInterpolatedSpectrum::fromInterleaved(sony_ilce_9_b, 35, false) }
};

const LinearInterpolatedSpectrum* Spectra::getNamedSpectrum(const str& name)
{
    auto it = namedSpectra.find(name);
    if (it == namedSpectra.end())
    {
        return nullptr;
    }
    return &it->second;
}
