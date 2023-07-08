/// @file      imguiInterface.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <app/imguiInterface.hpp>
#include <imstb_truetype.h>
#include <vulkan/commandPool.hpp>
#include <vulkan/device.hpp>
#include <vulkan/framebuffer.hpp>
#include <vulkan/immediateSubmit.hpp>
#include <vulkan/instance.hpp>
#include <vulkan/renderPass.hpp>
#include <vulkan/swapchain.hpp>

using namespace rayce;

ImguiInterface::ImguiInterface(const std::unique_ptr<Instance>& instance, const std::unique_ptr<Device>& logicalDevice, const std::unique_ptr<CommandPool>& commandPool,
                               const std::unique_ptr<Swapchain>& swapchain, GLFWwindow* nativeWindowHandle)
    : mLVkLogicalDeviceRef(logicalDevice->getVkDevice())
    , mSwapchainExtent(swapchain->getSwapExtent())
{
    // Copied from example - lower count
    VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 10 },
                                          { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
                                          { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10 },
                                          { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10 },
                                          { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 10 },
                                          { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 10 },
                                          { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
                                          { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
                                          { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
                                          { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 10 },
                                          { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10 } };
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets       = 10 * std::size(pool_sizes);
    pool_info.poolSizeCount = static_cast<uint32>(std::size(pool_sizes));
    pool_info.pPoolSizes    = pool_sizes;
    RAYCE_CHECK_VK(vkCreateDescriptorPool(mLVkLogicalDeviceRef, &pool_info, nullptr, &mVkDescriptorPool), "Creating Imgui descriptor pool failed!");

    pRenderPass = std::make_unique<RenderPass>(logicalDevice, swapchain, VK_ATTACHMENT_LOAD_OP_LOAD);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    // No viewport for now - we run into warnings
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows
    // io.ConfigViewportsNoAutoMerge = true;
    // io.ConfigViewportsNoTaskBarIcon = true;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding              = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForVulkan(nativeWindowHandle, true);

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance       = instance->getVkInstance();
    initInfo.PhysicalDevice = logicalDevice->getVkPhysicalDevice();
    initInfo.Device         = mLVkLogicalDeviceRef;
    initInfo.Queue          = logicalDevice->getVkGraphicsQueue();
    initInfo.DescriptorPool = mVkDescriptorPool;
    initInfo.Subpass        = 0;
    initInfo.MinImageCount  = 3;
    initInfo.ImageCount     = 3;
    initInfo.MSAASamples    = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&initInfo, pRenderPass->getVkRenderPass());

    ImmediateSubmit::Execute(logicalDevice, commandPool, [&](VkCommandBuffer cmd) { ImGui_ImplVulkan_CreateFontsTexture(cmd); });

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

ImguiInterface::~ImguiInterface()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (mVkDescriptorPool)
    {
        vkDestroyDescriptorPool(mLVkLogicalDeviceRef, mVkDescriptorPool, nullptr);
    }
}

void ImguiInterface::begin()
{
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
}

void ImguiInterface::end(VkCommandBuffer commandBuffer, const std::unique_ptr<Framebuffer>& framebuffer, const std::vector<VkClearValue>& clearValues)
{
    ImGui::Render();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = pRenderPass->getVkRenderPass();
    renderPassInfo.framebuffer       = framebuffer->getVkFramebuffer();
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = mSwapchainExtent;
    renderPassInfo.clearValueCount   = static_cast<uint32>(clearValues.size());
    renderPassInfo.pClearValues      = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    vkCmdEndRenderPass(commandBuffer);
}

void ImguiInterface::platformWindows()
{
    ImGuiIO io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}
