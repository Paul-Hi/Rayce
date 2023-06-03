/// @file      rayceApp.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include "rayceApp.hpp"

using namespace rayce;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                         const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    RAYCE_LOG_ERROR("VK Validation Layer: %s!", pCallbackData->pMessage);

    // Always return VK_FALSE
    return VK_FALSE;
}

static VkDebugUtilsMessengerCreateInfoEXT sDebugUtilsCreateInfo = {
    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    nullptr,
    0,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EX
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    debugUtilsCallback,
    nullptr
};

RayceApp::RayceApp(const RayceOptions& options)
{
    mAppState      = std::make_unique<RayceAppState>();
    mInternalState = std::make_unique<RayceInternalState>();

    mInternalState->windowWidth  = options.windowWidth;
    mInternalState->windowHeight = options.windowHeight;
    mInternalState->customGui    = options.customGui;

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    mInternalState->pWindow = glfwCreateWindow(mInternalState->windowWidth, mInternalState->windowHeight, "RayceApp", nullptr, nullptr);

    RAYCE_CHECK_NOTNULL(mInternalState->pWindow, "Creating GLFW window failed!");

    RAYCE_LOG_INFO("Created window of size (%d, %d)!", mInternalState->windowWidth, mInternalState->windowHeight);
}

bool RayceApp::initializeVulkan()
{
    bool success = true;

    // Initialize the vulkan instance
    success &= createVkInstance();

    if (!success)
        return false;

    return true;
}

bool RayceApp::run()
{
    while (!glfwWindowShouldClose(mInternalState->pWindow))
    {
        glfwPollEvents();
        // TODO: Call when imgui requires it obviously...
        mInternalState->customGui(mAppState);
    }
    return true;
}

bool RayceApp::shutdown()
{
    if (mInternalState->vkDebugMessenger)
    {
        destroyDebugUtilsMessenger(mInternalState->vkInstance, mInternalState->vkDebugMessenger, nullptr);
    }
    if (mInternalState->vkInstance)
    {
        vkDestroyInstance(mInternalState->vkInstance, nullptr);
    }

    glfwDestroyWindow(mInternalState->pWindow);
    glfwTerminate();

    RAYCE_LOG_INFO("Shutdown RayceApp!");

    return true;
}

bool rayce::RayceApp::createVkInstance()
{
    bool enableRequestedValidationLayers = false;
    if (mInternalState->enableValidationLayers)
    {
        enableRequestedValidationLayers = checkVkValidationLayers();
    }

    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "RayceApp";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "Rayce";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    provideRequiredExtensions(enableRequestedValidationLayers);

    createInfo.enabledExtensionCount   = static_cast<uint32>(mInternalState->instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = mInternalState->instanceExtensions.data();

    if (enableRequestedValidationLayers)
    {
        createInfo.enabledLayerCount   = static_cast<uint32_t>(mInternalState->validationLayers.size());
        createInfo.ppEnabledLayerNames = mInternalState->validationLayers.data();

        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&sDebugUtilsCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    // Create the instance
    if (vkCreateInstance(&createInfo, nullptr, &mInternalState->vkInstance) != VK_SUCCESS)
    {
        RAYCE_LOG_WARN("Creating Vulkan instance failed!");
        return false;
    }

    RAYCE_LOG_INFO("Created Vulkan instance!");

    if (enableRequestedValidationLayers)
    {
        createDebugMessenger();
    }

    return true;
}

bool rayce::RayceApp::createDebugMessenger()
{
    createDebugUtilsMessenger(mInternalState->vkInstance, &sDebugUtilsCreateInfo, nullptr, &mInternalState->vkDebugMessenger);

    RAYCE_LOG_INFO("Created debug messenger!");

    return true;
}

VkResult rayce::RayceApp::createDebugUtilsMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                                                    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void rayce::RayceApp::destroyDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

bool rayce::RayceApp::provideRequiredExtensions(bool enableRequestedValidationLayers)
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    mInternalState->instanceExtensions = std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableRequestedValidationLayers)
    {
        mInternalState->instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return true;
}

bool rayce::RayceApp::checkVkValidationLayers()
{
#ifdef RAYCE_DEBUG
    uint32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : mInternalState->validationLayers)
    {
        bool layerFound = false;

        for (const VkLayerProperties& layerProperties : availableLayers)
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
#else
    return true;
#endif
}
