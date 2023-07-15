/// @file      scene.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef SCENE_HPP
#define SCENE_HPP

#include <core/export.hpp>
#include <core/macro.hpp>
#include <core/types.hpp>

namespace rayce
{
    struct SceneReflectionInfo
    {
        str filename;
        std::vector<str> shapeNames;
        std::vector<uint32> shapeTriCounts;
    };

    class RAYCE_API_EXPORT Scene
    {
    public:
        Scene();
        ~Scene() = default;

        void loadFromObj(const str& filename, const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class CommandPool>& commandPool);

        const std::unique_ptr<class Geometry>& getGeometry()
        {
            return pGeometry;
        }

        uint32 maxVertex()
        {
            return mMaxVertex;
        }

        uint32 primitiveCount()
        {
            return mPrimitiveCount;
        }

        void onImGuiRender();

    private:
        std::unique_ptr<class Geometry> pGeometry;

        uint32 mMaxVertex;
        uint32 mPrimitiveCount;

        bool mReflectionOpen;

        SceneReflectionInfo mReflectionInfo;
    };

} // namespace rayce

#endif // SCENE_HPP
