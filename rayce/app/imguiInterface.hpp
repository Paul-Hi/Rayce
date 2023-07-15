/// @file      imguiInterface.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef IMGUI_INTERFACE_HPP
#define IMGUI_INTERFACE_HPP

#include <core/export.hpp>
#include <core/macro.hpp>
#include <core/types.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT ImguiInterface
    {
    public:
        ImguiInterface(const std::unique_ptr<class Instance>& instance, const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class CommandPool>& commandPool,
                       const std::unique_ptr<class Swapchain>& swapchain, GLFWwindow* nativeWindowHandle);
        ~ImguiInterface();

        void begin();
        void end(VkCommandBuffer commandBuffer, const std::unique_ptr<class Framebuffer>& framebuffer, const std::vector<VkClearValue>& clearValues);

        void platformWindows();

    private:
        VkDescriptorPool mVkDescriptorPool;
        VkDevice mLVkLogicalDeviceRef;
        VkExtent2D mSwapchainExtent;

        std::unique_ptr<class RenderPass> pRenderPass;
    };
} // namespace rayce

#endif // IMGUI_INTERFACE_HPP
