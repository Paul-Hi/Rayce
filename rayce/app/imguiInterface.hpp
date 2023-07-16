/// @file      imguiInterface.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef IMGUI_INTERFACE_HPP
#define IMGUI_INTERFACE_HPP

namespace rayce
{
    /// @brief Responsible beginning and ending an ImGui frame.
    class RAYCE_API_EXPORT ImguiInterface
    {
    public:
        /// @brief Constructs a new \a ImguiInterface.
        /// @param instance The vulkan \a Instance.
        /// @param logicalDevice The logical \a Device.
        /// @param commandPool The \a CommandPool to use.
        /// @param swapchain The current device \a Swapchain.
        /// @param nativeWindowHandle The native platform window handle.
        ImguiInterface(const std::unique_ptr<class Instance>& instance, const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class CommandPool>& commandPool,
                       const std::unique_ptr<class Swapchain>& swapchain, GLFWwindow* nativeWindowHandle);
        /// @brief Destructor.
        ~ImguiInterface();

        /// @brief Begins the new ImGui frame.
        void begin();

        /// @brief Ends the created ImGui frame.
        /// @param commandBuffer The \a CommandBuffer to put all draw commands into.
        /// @param framebuffer The \a Framebuffer to render into.
        /// @param clearValues The clear values to use when loading the \a Framebuffer.
        void end(VkCommandBuffer commandBuffer, const std::unique_ptr<class Framebuffer>& framebuffer, const std::vector<VkClearValue>& clearValues);

        /// @brief Updates platform windows when using multiple windows in ImGui.
        void platformWindows();

    private:
        /// @brief Handle of the descriptor pool used by ImGui.
        VkDescriptorPool mVkDescriptorPool;
        /// @brief Handle reference to the vulkan device.
        VkDevice mLVkLogicalDeviceRef;
        /// @brief The current swapchains extent.
        VkExtent2D mSwapchainExtent;

        /// @brief The \a RenderPass the ImGui should utilize.
        std::unique_ptr<class RenderPass> pRenderPass;
    };
} // namespace rayce

#endif // IMGUI_INTERFACE_HPP
