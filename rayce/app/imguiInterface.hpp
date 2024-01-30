/// @file      imguiInterface.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#ifndef IMGUI_INTERFACE_HPP
#define IMGUI_INTERFACE_HPP

#include <imgui.h>

namespace rayce
{
    namespace theme
    {
        const ImColor black(0, 0, 0, 255);
        const ImColor white(255, 255, 255, 255);
        const ImColor brighten(255, 255, 255, 127);
        const ImColor darken(0, 0, 0, 127);
        const ImColor header(47, 47, 47, 255);
        const ImColor text(192, 192, 192, 255);
        const ImColor titlebar(21, 21, 21, 255);
        const ImColor titlebarCollapsed(9, 9, 9, 255);
        const ImColor background(36, 36, 36, 255);
        const ImColor backgroundDark(26, 26, 26, 255);
        const ImColor backgroundProperty(15, 15, 15, 255);
        const ImColor backgroundGrab(32, 32, 32, 255);
        const ImColor backgroundPopup(50, 50, 50, 255);
        const ImColor accentClick(104, 104, 104, 255);

        const ImColor textError(236, 50, 50, 255);
        const ImColor highlight(36, 180, 241, 255);
        const ImColor activeGrab(42, 125, 157, 255);
    }

    /// @brief Responsible beginning and ending an ImGui frame.
    class RAYCE_API_EXPORT ImguiInterface
    {
    public:
        /// @brief Constructs a new @a ImguiInterface.
        /// @param[in] instance The vulkan @a Instance.
        /// @param[in] logicalDevice The logical @a Device.
        /// @param[in] commandPool The @a CommandPool to use.
        /// @param[in] swapchain The current device @a Swapchain.
        /// @param[in] nativeWindowHandle The native platform window handle.
        ImguiInterface(const std::unique_ptr<class Instance>& instance, const std::unique_ptr<class Device>& logicalDevice, const std::unique_ptr<class CommandPool>& commandPool,
                       const std::unique_ptr<class Swapchain>& swapchain, GLFWwindow* nativeWindowHandle);
        /// @brief Destructor.
        ~ImguiInterface();

        /// @brief Begins the new ImGui frame.
        /// @param[in] window The application window.
        void begin(const std::unique_ptr<class Window>& window);

        /// @brief Ends the created ImGui frame.
        /// @param[in] commandBuffer The @a CommandBuffer to put all draw commands into.
        /// @param[in] framebuffer The @a Framebuffer to render into.
        /// @param[in] clearValues The clear values to use when loading the @a Framebuffer.
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

        /// @brief The @a RenderPass the ImGui should utilize.
        std::unique_ptr<class RenderPass> pRenderPass;

        /// @brief Sets the Imgui style - default setup done in code, settings could be exposed.
        /// @param[in] scaleFactorFont The font scaling factor to apply.
        void setupImGuiStyle(float scaleFactorFont);
    };
} // namespace rayce

#endif // IMGUI_INTERFACE_HPP
