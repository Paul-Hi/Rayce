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

        const std::unique_ptr<class Window>& getWindow() const
        {
            return pWindow;
        }

        const std::unique_ptr<class Instance>& getInstance() const
        {
            return pInstance;
        }

        const std::unique_ptr<class Device>& getDevice() const
        {
            return pDevice;
        }

        const std::unique_ptr<class Surface>& getSurface() const
        {
            return pSurface;
        }

        virtual bool onInitialize();
        virtual bool onShutdown();
        virtual void onUpdate();
        virtual void onRender();
        virtual void onImGuiRender();

        VkPhysicalDevice pickPhysicalDevice();

      private:
        int32 mWindowWidth;
        int32 mWindowHeight;
        bool mEnableValidationLayers;

        std::unique_ptr<class Window> pWindow;
        std::unique_ptr<class Instance> pInstance;
        std::unique_ptr<class Surface> pSurface;
        std::unique_ptr<class Device> pDevice;
        std::unique_ptr<class Swapchain> pSwapchain;
        std::unique_ptr<class GraphicsPipeline> pGraphicsPipeline;

        std::vector<const char*> mValidationLayers = { "VK_LAYER_KHRONOS_validation" };
    };
} // namespace rayce

#endif // RAYCE_APP_HPP
