/// @file      spectrum.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#pragma once

// A mixture between mitsuba, pbrtv4 and Falcor

#ifndef RAYCE_SPECTRUM_HPP
#define RAYCE_SPECTRUM_HPP

#include <core/color.hpp>
#include <core/utils.hpp>
#include <numeric>

#define SPECTRUM_SAMPLES 3 // RGB

#define SPECTRUM_MIN_WAVELENGTH 360
#define SPECTRUM_MAX_WAVELENGTH 830

namespace rayce
{
    class RAYCE_API_EXPORT Spectrum
    {
    public:
        virtual float evaluate(float lambda) const = 0;

        virtual vec2 range() const = 0;

        virtual bool empty() const = 0;

        virtual ~Spectrum() = default;
    };

    class RAYCE_API_EXPORT ConstantSpectrum : public Spectrum
    {
    public:
        ConstantSpectrum(float c)
            : mConstant(c)
        {
        }

        float evaluate(float lambda) const override
        {
            return mConstant;
        }

        bool empty() const override
        {
            return false;
        }

        vec2 range() const override
        {
            return vec2(SPECTRUM_MIN_WAVELENGTH, SPECTRUM_MAX_WAVELENGTH);
        }

        virtual ~ConstantSpectrum() = default;

    private:
        float mConstant;
    };

    class RAYCE_API_EXPORT DenseSpectrum : public Spectrum
    {
    public:
        DenseSpectrum(float lambdaMin = SPECTRUM_MIN_WAVELENGTH, float lambdaMax = SPECTRUM_MAX_WAVELENGTH)
            : mRangeLambda(lambdaMin, lambdaMax)
            , mValues(lambdaMax - lambdaMin + 1)
        {
        }

        DenseSpectrum(float lambdaMin, float lambdaMax, const std::vector<float> values)
            : mRangeLambda(lambdaMin, lambdaMax)
            , mValues(values)
        {
        }

        DenseSpectrum(float lambdaMin, float lambdaMax, const float* values, ptr_size count)
            : mRangeLambda(lambdaMin, lambdaMax)
            , mValues(values, values + count)
        {
        }

        DenseSpectrum(float lambdaMin, float lambdaMax, const Spectrum& spectrum)
            : mRangeLambda(lambdaMin, lambdaMax)
            , mValues(lambdaMax - lambdaMin + 1)
        {
            if (!spectrum.empty())
            {
                for (float lambda = mRangeLambda.x(); lambda <= mRangeLambda.y(); lambda += 1.0)
                {
                    int index      = std::lround(lambda - mRangeLambda.x());
                    mValues[index] = spectrum.evaluate(lambda);
                }
            }
        }

        DenseSpectrum(float lambdaMin, float lambdaMax, const std::function<float(float)>& func)
            : mRangeLambda(lambdaMin, lambdaMax)
            , mValues(lambdaMax - lambdaMin + 1)
        {
            for (float lambda = mRangeLambda.x(); lambda <= mRangeLambda.y(); lambda += 1.0)
            {
                int index      = std::lround(lambda - mRangeLambda.x());
                mValues[index] = func(lambda);
            }
        }

        DenseSpectrum(const DenseSpectrum& other)
            : mRangeLambda(other.mRangeLambda)
            , mValues(other.mValues)
        {
        }

        float evaluate(float lambda) const override
        {
            int index = std::lround(lambda - mRangeLambda.x());
            if (index < 0 || index >= mValues.size())
            {
                return 0.0;
            }

            return mValues[index];
        }

        bool empty() const override
        {
            return mValues.empty() || mValues[0] < 0.0;
        }

        vec2 range() const override
        {
            return mRangeLambda;
        }

        void scale(float f)
        {
            for (float& v : mValues)
            {
                v *= f;
            }
        }

        virtual ~DenseSpectrum() = default;

    private:
        vec2 mRangeLambda;
        std::vector<float> mValues;
    };

    class RAYCE_API_EXPORT LinearInterpolatedSpectrum : public Spectrum
    {
    public:
        LinearInterpolatedSpectrum(const std::vector<float>& lambdas, const std::vector<float>& values)
        {
            assert(lambdas.size() == values.size());

            std::vector<uint32> indices(lambdas.size());
            std::iota(indices.begin(), indices.end(), 0);

            struct IndexSorter
            {
            public:
                IndexSorter(const float* v)
                    : mV(v)
                {
                }

                bool operator()(int i, int j) const { return mV[i] < mV[j]; }

            private:
                const float* mV;
            };

            std::sort(indices.begin(), indices.end(), IndexSorter(lambdas.data()));

            for (uint32& index : indices)
            {
                mLambdas.push_back(lambdas[index]);
                mValues.push_back(values[index]);
            }

            // everything sorted :)
        }

        float evaluate(float lambda) const override
        {
            if (mLambdas.empty() || lambda < mLambdas.front() || lambda > mLambdas.back())
            {
                return 0.0;
            }

            uint32 index = interval(mLambdas.size(), [&](uint32 idx)
                                    { return mLambdas[idx] <= lambda; });

            float t = (lambda - mLambdas[index]) / (mLambdas[index + 1] - mLambdas[index]);

            return (1.0 - t) * mValues[index] + t * mValues[index + 1];
        }

        bool empty() const override
        {
            return mLambdas.empty();
        }

        vec2 range() const override
        {
            return vec2(mLambdas.front(), mLambdas.back());
        }

        void scale(float f)
        {
            for (float& v : mValues)
            {
                v *= f;
            }
        }

        static LinearInterpolatedSpectrum fromInterleaved(const float* interleaved, ptr_size count, bool normalize);

        // for mitsuba spd files - there might be some duplicates with the interleaved stuff from pbrt, but hey...
        static LinearInterpolatedSpectrum fromFile(const str& filename);

        virtual ~LinearInterpolatedSpectrum() = default;

    private:
        std::vector<float> mLambdas;
        std::vector<float> mValues;
    };

    class RAYCE_API_EXPORT RGBSpectrum : public Spectrum
    {
        RGBSpectrum(const vec3 rgb)
            : mRGB(rgb)
        {
        }

        float evaluate(float lambda) const override
        {
            if (lambda == 700.0)
                return mRGB.x();
            if (lambda == 546.1)
                return mRGB.y();
            if (lambda == 435.8)
                return mRGB.z();

            return 0.0;
        }

        bool empty() const override
        {
            return false;
        }

        vec2 range() const override
        {
            return vec2(435.8, 700.0);
        }

        vec3 rgb() const
        {
            return mRGB;
        }

        virtual ~RGBSpectrum() = default;

    private:
        vec3 mRGB;
    };

    class RAYCE_API_EXPORT BlackBodySpectrum : public Spectrum
    {
    public:
        BlackBodySpectrum(float temperature)
            : mTemp(temperature)
        {
            float lambdaMax = 2.8977721e-3 / temperature;
            mNormalization  = 1.0 / blackbody(lambdaMax * 1e9, temperature);
        }

        float evaluate(float lambda) const override
        {
            return blackbody(lambda, mTemp) * mNormalization;
        }

        bool empty() const override
        {
            return false;
        }

        vec2 range() const override
        {
            return vec2(SPECTRUM_MIN_WAVELENGTH, SPECTRUM_MAX_WAVELENGTH);
        }

        virtual ~BlackBodySpectrum() = default;

    private:
        static constexpr float c  = 299792458.0;    // speed of light in vaccum
        static constexpr float h  = 6.62606957e-34; // planck's constant
        static constexpr float kb = 1.3806488e-23;  // boltzmann's constant

        float blackbody(float lambda, float temperature) const
        {
            if (temperature <= 0.0)
            {
                return 0.0;
            }
            float l  = lambda * 1e-9;
            float Le = (2.0 * h * c * c) / (l * l * l * l * l * (exp((h * c) / (l * kb * temperature)) - 1.0));
            return Le;
        }

    private:
        float mTemp;
        float mNormalization;
    };

    template <typename SpecA, typename SpecB>
    float RAYCE_API_EXPORT innerProduct(const SpecA& f, const SpecB& g)
    {
        auto rangeA     = f.range();
        auto rangeB     = g.range();
        float minLambda = std::max(rangeA.x(), rangeB.x());
        float maxLambda = std::min(rangeA.y(), rangeB.y());
        float integral  = 0.0;
        for (float lambda = minLambda; lambda <= maxLambda; lambda += 1.0)
        {
            integral += f.evaluate(lambda) * g.evaluate(lambda);
        }
        return integral;
    }

    struct RAYCE_API_EXPORT Spectra
    {
        // CIE 1931
        static const DenseSpectrum CIEX;
        static const DenseSpectrum CIEY;
        static const DenseSpectrum CIEZ;
        static constexpr float CIEYIntegral = 106.856895;

        static const LinearInterpolatedSpectrum* getNamedSpectrum(const str& name);
    };

    template <typename Spec>
    vec3 RAYCE_API_EXPORT spectrumToXYZ(const Spec& spectrum)
    {
        return vec3(innerProduct(spectrum, Spectra::CIEX), innerProduct(spectrum, Spectra::CIEY), innerProduct(spectrum, Spectra::CIEZ)) / Spectra::CIEYIntegral;
    }

    template <typename Spec>
    vec3 RAYCE_API_EXPORT spectrumToRGB(const Spec& spectrum)
    {
        return XYZtoRGBRec709(spectrumToXYZ(spectrum));
    }

    // FIXME: To continue: support the stuff that falcor supports -> import spectrum -> convert to rgb -> load rgb to shader --- handle spectral rendering later :D
    // https://github.com/NVIDIAGameWorks/Falcor/blob/036a44b1d8a1ce0c8848883e114fedbdc30c78ec/Source/plugins/importers/PBRTImporter/PBRTImporter.cpp - getSpectrumAsRGB() and spectrumToRGB()
};

#endif // RAYCE_SPECTRUM_HPP