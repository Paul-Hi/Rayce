/// @file      instance.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef INSTANCE_HPP
#define INSTANCE_HPP

#include <export.hpp>
#include <macro.hpp>
#include <types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT Instance
    {
      public:
        DISABLE_COPY_MOVE_VK(Instance)

        Instance(bool enableValidationLayers, std::vector<const char*>& deviceExtensions, std::vector<const char*>& validationLayers);
        ~Instance();

        VkInstance getVkInstance() const
        {
            return mVkInstance;
        }

        VkDebugUtilsMessengerEXT getVkDebugMessenger() const
        {
            return mVkDebugMessenger;
        }

        const std::vector<VkExtensionProperties>& getAvailableVkExtensions() const
        {
            return mVkExtensions;
        }

        const std::vector<VkLayerProperties>& getAvailableVkLayers() const
        {
            return mVkLayers;
        }

        const std::vector<VkPhysicalDevice>& getAvailableVkPhysicalDevices() const
        {
            return mVkPhysicalDevices;
        }

        const std::vector<const char*>& getEnabledValidationLayers() const
        {
            return mEnabledValidationLayers;
        }

      private:
        VkInstance mVkInstance;
        VkDebugUtilsMessengerEXT mVkDebugMessenger;

        std::vector<VkExtensionProperties> mVkExtensions;
        std::vector<VkLayerProperties> mVkLayers;
        std::vector<VkPhysicalDevice> mVkPhysicalDevices;

        std::vector<const char*> mEnabledValidationLayers;

        bool checkVkValidationLayers(std::vector<const char*>& validationLayers);
        std::vector<const char*> provideRequiredExtensions(bool enableRequestedValidationLayers, const std::vector<const char*>& deviceExtensions);
        void createDebugMessenger();
        void createVkInstance(bool enableValidationLayers, std::vector<const char*>& deviceExtensions, std::vector<const char*>& validationLayers);

        // Extension functions have to be loaded - we wrap here, since we only call these once.
        VkResult createDebugUtilsMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                                           VkDebugUtilsMessengerEXT* pDebugMessenger);
        void destroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    };
} // namespace rayce

#endif // INSTANCE_HPP
