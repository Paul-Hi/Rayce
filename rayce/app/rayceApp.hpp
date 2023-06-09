/// @file      rayceApp.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#ifndef RAYCE_APP_HPP
#define RAYCE_APP_HPP

#include <core/export.hpp>
#include <core/macro.hpp>
#include <core/types.hpp>

namespace rayce
{
    /// @brief Options for creating any \a RayceApp.
    struct RAYCE_API_EXPORT RayceOptions
    {
        /// @brief The name of the application.
        str name;
        /// @brief The width of the applications window.
        int32 windowWidth;
        /// @brief The height of the applications window.
        int32 windowHeight;

        /// @brief True, if validation layers in vulkan should be enabled, else false.
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

        const std::unique_ptr<class Swapchain>& getSwapchain() const
        {
            return pSwapchain;
        }

        const std::unique_ptr<class GraphicsPipeline>& getGraphicsPipeline() const
        {
            return pGraphicsPipeline;
        }

        const std::unique_ptr<class RaytracingPipeline>& getRaytracingPipeline() const
        {
            return pRaytracingPipeline;
        }

        const std::unique_ptr<class CommandPool>& getCommandPool() const
        {
            return pCommandPool;
        }

        const std::unique_ptr<class CommandBuffers>& getCommandBuffers() const
        {
            return pCommandBuffers;
        }

        const std::unique_ptr<class ImguiInterface>& getImguiInterface() const
        {
            return pImguiInterface;
        }

        const std::unique_ptr<class Geometry>& getGeometry() const
        {
            return pGeometry;
        }

        const std::vector<std::unique_ptr<class Framebuffer>>& getSwapchainFramebuffers() const
        {
            return mSwapchainFramebuffers;
        }

        const std::vector<std::unique_ptr<class AccelerationStructure>>& getBLAS() const
        {
            return mBLAS;
        }

        const std::unique_ptr<class AccelerationStructure>& getTLAS() const
        {
            return pTLAS;
        }

        const std::unique_ptr<class Image>& getRaytracingTargetImage() const
        {
            return pRaytracingTargetImage;
        }

        const std::unique_ptr<class ImageView>& getRaytracingTargetView() const
        {
            return pRaytracingTargetView;
        }

        const std::vector<std::unique_ptr<class Semaphore>>& getImageAvailableSemaphores() const
        {
            return mImageAvailableSemaphores;
        }

        const std::vector<std::unique_ptr<class Semaphore>>& getRenderFinishedSemaphores() const
        {
            return mRenderFinishedSemaphores;
        }

        const std::vector<std::unique_ptr<class Fence>>& getInFlightFences() const
        {
            return mInFlightFences;
        }

        uint32& getCurrentFrame()
        {
            return mCurrentFrame;
        }

        std::unique_ptr<class RTFunctions> pRTF;

        virtual bool onInitialize();
        virtual bool onShutdown();
        virtual void onUpdate();
        virtual void onFrameDraw();
        virtual void onRender(VkCommandBuffer commandBuffer, const uint32 imageIndex);
        virtual void onImGuiRender(VkCommandBuffer commandBuffer, const uint32 imageIndex);

        VkPhysicalDevice pickPhysicalDevice(bool& raytracingSupported);

      private:
        int32 mWindowWidth;
        int32 mWindowHeight;
        bool mEnableValidationLayers;
        VkPhysicalDevice mPhysicalDevice;

        std::unique_ptr<class Window> pWindow;
        std::unique_ptr<class Instance> pInstance;
        std::unique_ptr<class Surface> pSurface;
        std::unique_ptr<class Device> pDevice;
        std::unique_ptr<class Swapchain> pSwapchain;
        std::unique_ptr<class GraphicsPipeline> pGraphicsPipeline;
        std::unique_ptr<class RaytracingPipeline> pRaytracingPipeline;
        std::unique_ptr<class CommandPool> pCommandPool;
        std::unique_ptr<class CommandBuffers> pCommandBuffers;
        std::unique_ptr<class ImguiInterface> pImguiInterface;
        std::unique_ptr<class Geometry> pGeometry;
        std::vector<std::unique_ptr<class Framebuffer>> mSwapchainFramebuffers;

        std::vector<std::unique_ptr<class AccelerationStructure>> mBLAS;
        std::unique_ptr<class AccelerationStructure> pTLAS;

        std::unique_ptr<class Image> pRaytracingTargetImage;
        std::unique_ptr<class ImageView> pRaytracingTargetView;

        std::vector<std::unique_ptr<class Semaphore>> mImageAvailableSemaphores;
        std::vector<std::unique_ptr<class Semaphore>> mRenderFinishedSemaphores;
        std::vector<std::unique_ptr<class Fence>> mInFlightFences;

        uint32 mCurrentFrame;
        std::vector<const char*> mValidationLayers = { "VK_LAYER_KHRONOS_validation" };

        void recreateSwapchain();
    };
} // namespace rayce

#endif // RAYCE_APP_HPP
