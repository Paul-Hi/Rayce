/// @file      rayceApp.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef RAYCE_APP_HPP
#define RAYCE_APP_HPP

#include <export.hpp>
#include <macro.hpp>
#include <types.hpp>
#include <vulkan/instance.hpp>
#include <vulkan/window.hpp>

namespace rayce
{
    struct RAYCE_API_EXPORT RayceOptions
    {
        str name;
        int32 windowWidth;
        int32 windowHeight;

        bool enableValidationLayers;
    };

    /// @brief Application.
    class RAYCE_API_EXPORT RayceApp
    {
      public:
        virtual ~RayceApp();

        bool run();

        int32 getWindowWidth()
        {
            return mWindowWidth;
        }

        int32 getWindowHeight()
        {
            return mWindowHeight;
        }

      protected:
        /// @brief Constructor.
        /// @param[in] options Options to setup the application.
        RayceApp(const RayceOptions& options);

        const std::unique_ptr<Window>& getWindow() const
        {
            return pWindow;
        }

        const std::unique_ptr<Instance>& getInstance() const
        {
            return pInstance;
        }

        virtual bool onInitialize();
        virtual bool onShutdown();
        virtual void onUpdate();
        virtual void onRender();
        virtual void onImGuiRender();

      private:
        int32 mWindowWidth;
        int32 mWindowHeight;
        bool mEnableValidationLayers;

        std::unique_ptr<Window> pWindow;
        std::unique_ptr<Instance> pInstance;
    };
} // namespace rayce

#endif // RAYCE_APP_HPP
