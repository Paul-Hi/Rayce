/// @file      device.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <set>
#include <vulkan/device.hpp>

using namespace rayce;

Device::Device(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const std::vector<const char*>& enabledValidationLayers, bool raytracingSupported)
    : mVkPhysicalDevice(physicalDevice)
{
    // We have already queried and computed all of these earlier...
    vkGetPhysicalDeviceProperties(physicalDevice, &mProperties);

    uint32 queueFamilyPropertyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

    std::vector<VkQueueFamilyProperties>::iterator graphicsFamily =
        std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
                     [](const VkQueueFamilyProperties& queueFamily)
                     { return queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT; });
    // We want a separate family for compute stuff
    std::vector<VkQueueFamilyProperties>::iterator computeFamily = std::find_if(
        queueFamilyProperties.begin(), queueFamilyProperties.end(),
        [](const VkQueueFamilyProperties& queueFamily)
        { return queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT); });

    // Present family - most of the time equal to graphics family
    std::vector<VkQueueFamilyProperties>::iterator presentFamily = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
                                                                                [&](const VkQueueFamilyProperties& queueFamily)
                                                                                {
                                                                                    VkBool32 presentSupport = false;
                                                                                    const uint32 i          = static_cast<uint32>(&*queueFamilyProperties.cbegin() - &queueFamily);
                                                                                    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
                                                                                    return queueFamily.queueCount > 0 && presentSupport;
                                                                                });

    mGraphicsFamilyIndex = static_cast<uint32>(graphicsFamily - queueFamilyProperties.begin());
    mComputeFamilyIndex  = static_cast<uint32>(computeFamily - queueFamilyProperties.begin());
    mPresentFamilyIndex  = static_cast<uint32>(presentFamily - queueFamilyProperties.begin());

    // Sometimes queues can be the same, so we reduce them to unique indices.
    const std::set<uint32> uniqueQueueFamilyIndices = { mGraphicsFamilyIndex, mComputeFamilyIndex, mPresentFamilyIndex };

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    for (uint32 queueFamilyIndex : uniqueQueueFamilyIndices)
    {
        queueCreateInfos.push_back(createVkQueue(queueFamilyIndex));
    }

    VkPhysicalDeviceFeatures physicalDeviceFeatures{};
    physicalDeviceFeatures.geometryShader    = VK_TRUE;
    physicalDeviceFeatures.shaderInt64       = VK_TRUE;
    physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

    // Swapchain has to be enabled.
    // Swapchain
    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    // Adding required extensions for raytracing here.
    if (raytracingSupported)
    {
        // Basic raytracing extension
        deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        // Required by VK_KHR_acceleration_structure
        deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
        deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
        // Required for VK_KHR_ray_tracing_pipeline
        deviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
        // Required by VK_KHR_spirv_1_4
        deviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
    }

    // additional device features for raytracing
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
    bufferDeviceAddressFeatures.sType               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    bufferDeviceAddressFeatures.pNext               = nullptr;
    bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;

    VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures{};
    indexingFeatures.sType                                     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    indexingFeatures.pNext                                     = &bufferDeviceAddressFeatures;
    indexingFeatures.runtimeDescriptorArray                    = VK_TRUE;
    indexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    // FIXME: The following need support checks?
    indexingFeatures.descriptorBindingPartiallyBound               = VK_TRUE;
    indexingFeatures.descriptorBindingVariableDescriptorCount      = VK_TRUE;
    indexingFeatures.descriptorBindingUpdateUnusedWhilePending     = VK_TRUE;
    indexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
    accelerationStructureFeatures.sType                 = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accelerationStructureFeatures.pNext                 = &indexingFeatures;
    accelerationStructureFeatures.accelerationStructure = VK_TRUE;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingFeatures{};
    rayTracingFeatures.sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rayTracingFeatures.pNext              = &accelerationStructureFeatures;
    rayTracingFeatures.rayTracingPipeline = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos    = queueCreateInfos.data();
    createInfo.pNext                = raytracingSupported ? &rayTracingFeatures : nullptr;
    createInfo.queueCreateInfoCount = static_cast<uint32>(queueCreateInfos.size());
    createInfo.pEnabledFeatures     = &physicalDeviceFeatures;

    createInfo.enabledExtensionCount   = static_cast<uint32>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // The following is not relevant for more recent vulkan versions - it is inherited from the instance there.
    if (!enabledValidationLayers.empty())
    {
        createInfo.enabledLayerCount   = static_cast<uint32_t>(enabledValidationLayers.size());
        createInfo.ppEnabledLayerNames = enabledValidationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    // Create the device
    RAYCE_CHECK_VK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &mVkDevice), "Creating logical device failed!");

    // Retrieve Queues
    vkGetDeviceQueue(mVkDevice, mGraphicsFamilyIndex, 0, &mVkGraphicsQueue);
    vkGetDeviceQueue(mVkDevice, mComputeFamilyIndex, 0, &mVkComputeQueue);
    vkGetDeviceQueue(mVkDevice, mPresentFamilyIndex, 0, &mVkPresentQueue);

    RAYCE_LOG_INFO("Created logical device!");
}

Device::~Device()
{
    if (mVkDevice)
    {
        vkDestroyDevice(mVkDevice, nullptr);
    }
}

VkDeviceQueueCreateInfo Device::createVkQueue(uint32 queueFamilyIndex)
{
    float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    queueCreateInfo.queueCount       = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    return queueCreateInfo;
}
