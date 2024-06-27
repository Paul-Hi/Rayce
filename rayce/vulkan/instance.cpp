/// @file      instance.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#include <vulkan/instance.hpp>

using namespace rayce;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                         const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    RAYCE_LOG_ERROR("VK Validation Layer: %s!", pCallbackData->pMessage);

    RAYCE_UNUSED(messageSeverity);
    RAYCE_UNUSED(messageType);
    RAYCE_UNUSED(pUserData);

    // Always return VK_FALSE
    return VK_FALSE;
}

static VkDebugUtilsMessengerCreateInfoEXT sDebugUtilsCreateInfo{
    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    nullptr,
    0,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT// | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
    ,VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    debugUtilsCallback,
    nullptr
};

Instance::Instance(bool enableValidationLayers, std::vector<const char*>& deviceExtensions, std::vector<const char*>& validationLayers)
{
    createVkInstance(enableValidationLayers, deviceExtensions, validationLayers);

    uint32 extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    mVkExtensions.resize(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, mVkExtensions.data());

    uint32 physicalDeviceCount;
    vkEnumeratePhysicalDevices(mVkInstance, &physicalDeviceCount, nullptr);

    mVkPhysicalDevices.resize(physicalDeviceCount);
    vkEnumeratePhysicalDevices(mVkInstance, &physicalDeviceCount, mVkPhysicalDevices.data());

    RAYCE_CHECK_GT(physicalDeviceCount, 0u, "No Vulkan compatible devices found!");
}

Instance::~Instance()
{
    if (mVkDebugMessenger)
    {
        destroyDebugUtilsMessenger(mVkInstance, mVkDebugMessenger, nullptr);
    }
    if (mVkInstance)
    {
        vkDestroyInstance(mVkInstance, nullptr);
    }
}

void Instance::createVkInstance(bool enableValidationLayers, std::vector<const char*>& deviceExtensions, std::vector<const char*>& validationLayers)
{
    bool enableRequestedValidationLayers = false;
    if (enableValidationLayers)
    {
        enableRequestedValidationLayers = checkVkValidationLayers(validationLayers);
    }

    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "RayceApp";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "Rayce";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> instanceExtensions = provideRequiredExtensions(enableRequestedValidationLayers, deviceExtensions);

    createInfo.enabledExtensionCount   = static_cast<uint32>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    if (enableRequestedValidationLayers)
    {
        mEnabledValidationLayers       = validationLayers;
        createInfo.enabledLayerCount   = static_cast<uint32_t>(mEnabledValidationLayers.size());
        createInfo.ppEnabledLayerNames = mEnabledValidationLayers.data();

        createInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&sDebugUtilsCreateInfo);
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    // Create the instance
    RAYCE_CHECK_VK(vkCreateInstance(&createInfo, nullptr, &mVkInstance), "Creating Vulkan instance failed!");

    RAYCE_LOG_INFO("Created Vulkan instance!");

    if (enableRequestedValidationLayers)
    {
        createDebugMessenger();
    }
}

void Instance::createDebugMessenger()
{
    RAYCE_TRY_VK(createDebugUtilsMessenger(mVkInstance, &sDebugUtilsCreateInfo, nullptr, &mVkDebugMessenger), "Creating debug messenger failed!");
}

std::vector<const char*> Instance::provideRequiredExtensions(bool enableRequestedValidationLayers, const std::vector<const char*>& deviceExtensions)
{
    std::vector<const char*> instanceExtensions = deviceExtensions;

    if (enableRequestedValidationLayers)
    {
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return instanceExtensions;
}

bool Instance::checkVkValidationLayers(std::vector<const char*>& validationLayers)
{
    uint32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    mVkLayers.resize(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, mVkLayers.data());

    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;

        for (const VkLayerProperties& layerProperties : mVkLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                RAYCE_LOG_INFO("Found validation layer %s!", layerName);
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            LOG_F(WARNING, "Requested validation layer %s is not available. Validation layers are disabled!", layerName);
            return false;
        }
    }

    RAYCE_LOG_INFO("Found all requested validation layers!");
    return true;
}

VkResult Instance::createDebugUtilsMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                                             VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void Instance::destroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}
