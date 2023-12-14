/// @file      spectrum.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

// A mixture between mitsuba, pbrtv4 and Falcor

#ifndef RAYCE_SPECTRUM_HPP
#define RAYCE_SPECTRUM_HPP

#define SPECTRUM_SAMPLES 3 // RGB

#define SPECTRUM_MIN_WAVELENGTH 360
#define SPECTRUM_MAX_WAVELENGTH 830
#define SPECTRUM_RANGE (SPECTRUM_MAX_WAVELENGTH - SPECTRUM_MIN_WAVELENGTH)

namespace rayce
{
    class RAYCE_API_EXPORT ContinuousSpectrum
    {
    public:
        virtual float evaluate(float lambda) const = 0;

        virtual float average(float lambdaMin, float lambdaMax) const;

        virtual vec2 range() const;

        virtual ~ContinuousSpectrum() = default;
    };

    class RAYCE_API_EXPORT InterpolatedSpectrum : public ContinuousSpectrum
    {
    public:
        InterpolatedSpectrum(ptr_size size = 0);

        InterpolatedSpectrum(const std::vector<float>& wavelengths, const std::vector<float>& values, ptr_size entries);

        InterpolatedSpectrum(const str& filename);

        void append(float lambda, float value);

        void clear();

        float evaluate(float lambda) const override;

        float average(float lambdaMin, float lambdaMax) const override;

        virtual ~InterpolatedSpectrum() = default;
    };

    class RAYCE_API_EXPORT DenseSpectrum : public ContinuousSpectrum
    {
    public:
        DenseSpectrum(float lambdaMin, float lambdaMax, const std::vector<float> values);

        template <typename Spec>
        DenseSpectrum(const Spec& spectrum, float step = 1.0);

        DenseSpectrum(const str& filename);

        void append(float lambda, float value);

        void clear();

        float evaluate(float lambda) const override;

        float average(float lambdaMin, float lambdaMax) const override;

        virtual ~DenseSpectrum() = default;
    };

    template <typename SpecA, typename SpecB>
    float innerProduct(const SpecA& a, const SpecB& b)
    {
        auto rangeA     = a.range();
        auto rangeB     = b.range();
        float minLambda = std::max(rangeA.x, rangeB.x);
        float maxLambda = std::min(rangeA.y, rangeB.y);
        float integral  = 0.0;
        for (float lambda = minLambda; lambda <= maxLambda; lambda += 1.0)
        {
            integral += a.eval(lambda) * b.eval(lambda);
        }
        return integral;
    }

    struct RAYCE_API_EXPORT Spectra
    {
        // CIE 1931
        static const DenseSpectrum kCIEX;
        static const DenseSpectrum kCIEY;
        static const DenseSpectrum kCIEZ;
        static constexpr float kCIEYIntegral = 106.856895;

        static const InterpolatedSpectrum getNamedSpectrum(const std::string& name);
    };

    template <typename Spec>
    vec3 spectrumToXYZ(const Spec& spectrum)
    {
        return vec3(innerProduct(spectrum, Spectra::kCIEX), innerProduct(spectrum, Spectra::kCIEY), innerProduct(spectrum, Spectra::kCIEZ)) / Spectra::kCIEYIntegral;
    }

    template <typename Spec>
    vec3 spectrumToRGB(const Spec& spectrum)
    {
        return XYZtoRGBRec709(spectrumToXYZ(spectrum));
    }

    // FIXME: To continue: support the stuff that falcor supports -> import spectrum -> convert to rgb -> load rgb to shader --- handle spectral rendering later :D
    // https://github.com/NVIDIAGameWorks/Falcor/blob/036a44b1d8a1ce0c8848883e114fedbdc30c78ec/Source/plugins/importers/PBRTImporter/PBRTImporter.cpp - getSpectrumAsRGB() and spectrumToRGB()
};

#endif // RAYCE_SPECTRUM_HPP