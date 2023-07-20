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
        /// @brief The names of all shapes (meshes).
        std::vector<str> shapeNames;
        /// @brief The triangle count of all shapes (meshes).
        std::vector<uint32> shapeTriCounts;
    };

    /// @brief The scene storage of the pathtracer.
    class RAYCE_API_EXPORT RayceScene
    {
    public:
        /// @brief Constructs a new @a RaceScene.
        RayceScene();
        /// @brief Destructor.
        ~RayceScene() = default;

        /// @brief Loads a model from an obj file and preprocesses it for @a Rayces use case.
        /// @param[in] filename The obj filename.
        /// @param[in] logicalDevice The logical @a Device used to create necessary GPU structures.
        /// @param[in] commandPool @a CommandPool to get command buffers.
        void loadFromObj(const str& filename, const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class CommandPool>& commandPool);

        /// @brief Returns the created @a Geometry of the @a RayceScene.
        /// @return The created @a Geometry of the @a RayceScene.
        const std::unique_ptr<class Geometry>& getGeometry()
        {
            return pGeometry;
        }

        /// @brief Retrieves the @a ImageView for a given material index.
        /// @param materialID The material id to receive the texture @a ImageView for.
        /// @return The @a ImageView for a given material index.
        const std::unique_ptr<class ImageView>& getTextureView(uint32 materialID)
        {
            return mImageViews[materialID];
        }

        /// @brief Returns the maximum vertex index.
        /// @return The maximum vertex index.
        uint32 maxVertex()
        {
            return mMaxVertex;
        }

        /// @brief Returns the number of triangles in the @a RayceScene.
        /// @return The number of triangles in the @a RayceScene.
        uint32 primitiveCount()
        {
            return mPrimitiveCount;
        }

        /// @brief Renders the @a SceneReflectionInfo in an ImGui window.
        void onImGuiRender();

    private:
        /// @brief The @a Geometry of the @a RayceScene.
        std::unique_ptr<class Geometry> pGeometry;

        /// @brief The maximum vertex index.
        uint32 mMaxVertex;
        /// @brief The number of triangles.
        uint32 mPrimitiveCount;

        /// @brief True if the reflection info window is open, else False.
        bool mReflectionOpen;

        /// @brief The @a SceneReflectionInfo.
        SceneReflectionInfo mReflectionInfo;

        /// @brief Image cache to remember already loaded textures.
        std::unordered_map<str, byte*> mImageCache;

        /// @brief List of \a Images representing textures of the loaded @a Geometry.
        std::vector<std::unique_ptr<class Image>> mImages;

        /// @brief List of \a ImageViews for the images.
        std::vector<std::unique_ptr<class ImageView>> mImageViews;
    };

} // namespace rayce

#endif // RAYCE_SCENE_HPP
