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

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace rayce
{
    struct RAYCE_API_EXPORT RayceAppState
    {
    };

    struct RayceInternalState
    {
        int32 windowWidth;
        int32 windowHeight;
        void (*customGui)(std::unique_ptr<RayceAppState>&);

        GLFWwindow* pWindow;
        VkInstance vkInstance;
        VkDebugUtilsMessengerEXT vkDebugMessenger;

        std::vector<const char*> instanceExtensions;
        const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

#ifdef RAYCE_DEBUG
        const bool enableValidationLayers = true;
#else
        const bool enableValidationLayers = false;
#endif
    };

    struct RAYCE_API_EXPORT RayceOptions
    {
        int32 windowWidth;
        int32 windowHeight;

        void (*customGui)(std::unique_ptr<RayceAppState>&);
    };

    /// @brief Application.
    class RayceApp
    {
      public:
        /// @brief Constructor.
        /// @param[in] options Options to setup the application.
        RayceApp(const RayceOptions& options);

        bool RAYCE_API_EXPORT initializeVulkan();

        bool RAYCE_API_EXPORT run();

        bool RAYCE_API_EXPORT shutdown();

      private:
        std::unique_ptr<RayceAppState> mAppState;
        std::unique_ptr<RayceInternalState> mInternalState;

        bool checkVkValidationLayers();
        bool provideRequiredExtensions(bool enableRequestedValidationLayers);
        bool createDebugMessenger();
        bool createVkInstance();

        // Extension functions have to be loaded - we wrap here, since we only call these once.
        VkResult createDebugUtilsMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                                           VkDebugUtilsMessengerEXT* pDebugMessenger);
        void destroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    };
} // namespace rayce

#endif // RAYCE_APP_HPP
