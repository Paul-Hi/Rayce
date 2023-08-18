/// @file      scene.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef RAYCE_SCENE_HPP
#define RAYCE_SCENE_HPP

#include <unordered_map>

namespace rayce
{
    /// @brief Stores reflection information for a @a RayceScene.
    struct SceneReflectionInfo
    {
        /// @brief The file name the scene was loaded from (currently obj).
        str filename;
        /// @brief The names of all meshes.
        std::vector<str> meshNames;
        /// @brief The triangle count of all meshes.
        std::vector<uint32> meshTriCounts;
    };

    /// @brief The scene storage of the pathtracer.
    class RAYCE_API_EXPORT RayceScene
    {
    public:
        /// @brief Constructs a new @a RaceScene.
        RayceScene();
        /// @brief Destructor.
        ~RayceScene();

        /// @brief Loads a model from a mitsuba file and preprocesses it for @a Rayces use case.
        /// @param[in] filename The mitsuba file filename.
        /// @param[in] logicalDevice The logical @a Device used to create necessary GPU structures.
        /// @param[in] commandPool @a CommandPool to get command buffers.
        /// @param[in] scale A scaling for the positions.
        void loadFromMitsubaFile(const str& filename, const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class CommandPool>& commandPool, float scale);

        /// @brief Returns the created @a Geometry of the @a RayceScene.
        /// @return The created @a Geometry of the @a RayceScene.
        const std::unique_ptr<class Geometry>& getGeometry()
        {
            return pGeometry;
        }

        /// @brief Retrieves the @a Materials.
        /// @return The @a Materials in the @a RayceScene.
        const std::vector<std::unique_ptr<struct Material>>& getMaterials()
        {
            return mMaterials;
        }

        /// @brief Retrieves the @a ImageViews.
        /// @return The @a ImageViews for all materials in the @a RayceScene.
        const std::vector<std::unique_ptr<class ImageView>>& getTextureViews()
        {
            return mImageViews;
        }

        /// @brief Retrieves the @a Samplers.
        /// @return The @a Samplers for all materials in the @a RayceScene.
        const std::vector<std::unique_ptr<class Sampler>>& getSamplers()
        {
            return mImageSamplers;
        }

        /// @brief Renders the @a SceneReflectionInfo in an ImGui window.
        void onImGuiRender();

    private:
        /// @brief The @a Geometry of the @a RayceScene.
        std::unique_ptr<class Geometry> pGeometry;

        /// @brief True if the reflection info window is open, else False.
        bool mReflectionOpen;

        /// @brief The @a SceneReflectionInfo.
        SceneReflectionInfo mReflectionInfo;

        /// @brief The list of @a Materials of the loaded @a Geometry.
        std::vector<std::unique_ptr<struct Material>> mMaterials;

        /// @brief Image cache to remember already loaded textures.
        std::unordered_map<str, byte*> mImageCache;

        /// @brief List of \a Images representing textures of the loaded @a Geometry.
        std::vector<std::unique_ptr<class Image>> mImages;

        /// @brief List of \a ImageViews for the images.
        std::vector<std::unique_ptr<class ImageView>> mImageViews;
        /// @brief List of \a Samplers for the images.
        std::vector<std::unique_ptr<Sampler>> mImageSamplers;
    };

} // namespace rayce

#endif // RAYCE_SCENE_HPP
