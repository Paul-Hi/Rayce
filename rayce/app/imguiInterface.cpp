/// @file      imguiInterface.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <app/icons_font_awesome_5.hpp>
#include <app/imguiInterface.hpp>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    // No viewport for now - we run into warnings
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
    io.ConfigViewportsNoAutoMerge   = false;
    io.ConfigViewportsNoTaskBarIcon = true;

    setupImGuiStyle();

    ImGui_ImplGlfw_InitForVulkan(nativeWindowHandle, true);

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance       = instance->getVkInstance();
    initInfo.PhysicalDevice = logicalDevice->getVkPhysicalDevice();
    initInfo.Device         = mLVkLogicalDeviceRef;
    initInfo.Queue          = logicalDevice->getVkGraphicsQueue();
    initInfo.DescriptorPool = mVkDescriptorPool;
    initInfo.Subpass        = 0;
    initInfo.MinImageCount  = swapchain->getImageCount();
    initInfo.ImageCount     = swapchain->getImageCount();
    initInfo.MSAASamples    = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&initInfo, pRenderPass->getVkRenderPass());

    ImmediateSubmit::Execute(logicalDevice, commandPool, [&](VkCommandBuffer cmd)
                             { ImGui_ImplVulkan_CreateFontsTexture(cmd); });

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

static float convertHueToRGB(float p, float q, float t)
{

    if (t < 0)
        t += 1;
    if (t > 1)
        t -= 1;
    if (t < 1. / 6)
        return p + (q - p) * 6 * t;
    if (t < 1. / 2)
        return q;
    if (t < 2. / 3)
        return p + (q - p) * (2. / 3 - t) * 6;

    return p;
}

static ImVec4 convertHSLToRGBA(float h, float s, float l, float a = 255.0f)
{
    ImVec4 result;

    float hZO = h / 360.0f;
    float sZO = s / 100.0f;
    float lZO = l / 100.0f;
    float aZO = a / 100.0f;

    result.w = aZO;
    if (0 == sZO)
    {
        result.x = result.y = result.z = lZO; // achromatic
    }
    else
    {
        float q  = lZO < 0.5 ? lZO * (1 + sZO) : lZO + sZO - lZO * sZO;
        float p  = 2 * lZO - q;
        result.x = convertHueToRGB(p, q, hZO + 1. / 3);
        result.y = convertHueToRGB(p, q, hZO);
        result.z = convertHueToRGB(p, q, hZO - 1. / 3);
    }

    return result;
}

void ImguiInterface::setupImGuiStyle()
{
    ImGuiIO& io = ImGui::GetIO();

    io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/JetBrainsMono-Medium.ttf", 18.0);

    static const ImWchar iconsRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig iconsConfig;
    iconsConfig.MergeMode  = true;
    iconsConfig.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, 16.0f, &iconsConfig, iconsRanges);

    io.ConfigWindowsMoveFromTitleBarOnly = true; // Only move from title bar

    // TODO: Expose settings.
    struct ImGuiStyleSetup
    {
        // Colors in HSL

        float textHue = 225.0f;
        float textSat = 40.0f;
        float textLum = 98.0f;

        float backHue = 220.0f;
        float backSat = 10.0f;
        float backLum = 18.0f;

        float primaryHue = 221.0f;
        float primarySat = 32.0f;
        float primaryLum = 53.0f;

        float secundaryHue = 220.0f;
        float secundarySat = 31.0f;
        float secundaryLum = 13.0f;

        float accentHue = 221.0f;
        float accentSat = 98.0f;
        float accentLum = 67.0f;

        float rounding       = 0.75f;
        float alphaThreshold = 1.0f;
    } setup;

    ImVec4 text      = convertHSLToRGBA(setup.textHue, setup.textSat, setup.textLum);
    ImVec4 back      = convertHSLToRGBA(setup.backHue, setup.backSat, setup.backLum);
    ImVec4 primary   = convertHSLToRGBA(setup.primaryHue, setup.primarySat, setup.primaryLum);
    ImVec4 secundary = convertHSLToRGBA(setup.secundaryHue, setup.secundarySat, setup.secundaryLum);
    ImVec4 accent    = convertHSLToRGBA(setup.accentHue, setup.accentSat, setup.accentLum);

    // Setup Dear ImGui style
    ImVec4* colors                         = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text]                  = ImVec4(text.x, text.y, text.z, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(text.x, text.y, text.z, 0.58f);
    colors[ImGuiCol_WindowBg]              = ImVec4(back.x, back.y, back.z, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(back.x, back.y, back.z, 1.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(back.x * 0.8f, back.y * 0.8f, back.z * 0.8f, 1.00f);
    colors[ImGuiCol_Border]                = ImVec4(text.x, text.y, text.z, 0.30f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(back.x, back.y, back.z, 0.31f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(back.x, back.y, back.z, 0.68f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(back.x, back.y, back.z, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(back.x, back.y, back.z, 1.0f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(primary.x, primary.y, primary.z, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(secundary.x, secundary.y, secundary.z, 1.0f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(back.x, back.y, back.z, 1.0f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(back.x, back.y, back.z, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(secundary.x, secundary.y, secundary.z, 0.31f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(primary.x, primary.y, primary.z, 0.78f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(accent.x, accent.y, accent.z, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(text.x, text.y, text.z, 0.80f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(secundary.x, secundary.y, secundary.z, 0.54f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(accent.x, accent.y, accent.z, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(secundary.x, secundary.y, secundary.z, 0.44f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(primary.x, primary.y, primary.z, 0.86f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(accent.x, accent.y, accent.z, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(secundary.x, secundary.y, secundary.z, 0.46f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(primary.x, primary.y, primary.z, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(accent.x, accent.y, accent.z, 0.86f);
    colors[ImGuiCol_Separator]             = ImVec4(primary.x, primary.y, primary.z, 0.44f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(primary.x, primary.y, primary.z, 0.86f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(accent.x, accent.y, accent.z, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(secundary.x, secundary.y, secundary.z, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(primary.x, primary.y, primary.z, 0.78f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(accent.x, accent.y, accent.z, 1.00f);
    colors[ImGuiCol_Tab]                   = colors[ImGuiCol_Header];
    colors[ImGuiCol_TabHovered]            = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_TabActive]             = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TabUnfocused]          = colors[ImGuiCol_Tab];
    colors[ImGuiCol_TabUnfocusedActive]    = colors[ImGuiCol_TabActive];
    colors[ImGuiCol_DockingPreview]        = colors[ImGuiCol_Header];
    colors[ImGuiCol_DockingEmptyBg]        = ImVec4(back.x * 0.4f, back.y * 0.4f, back.z * 0.4f, 1.00f);
    colors[ImGuiCol_PlotLines]             = ImVec4(text.x, text.y, text.z, 0.63f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(primary.x, primary.y, primary.z, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(text.x, text.y, text.z, 0.63f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(primary.x, primary.y, primary.z, 1.00f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(accent.x, accent.y, accent.z, 0.43f);
    colors[ImGuiCol_DragDropTarget]        = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavHighlight]          = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight] = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingDimBg]     = colors[ImGuiCol_Header];
    colors[ImGuiCol_ModalWindowDimBg]      = colors[ImGuiCol_Header];

    ImGuiStyle& style      = ImGui::GetStyle();
    style.FrameRounding    = setup.rounding;
    style.WindowRounding   = setup.rounding;
    style.ChildBorderSize  = 1.0f;
    style.FrameBorderSize  = 0.0f;
    style.PopupBorderSize  = 1.0f;
    style.WindowBorderSize = 0.0f;
    style.Alpha            = 1.0f;

    for (int i = 0; i < ImGuiCol_COUNT; i++)
    {
        const auto colorId = static_cast<ImGuiCol>(i);
        auto& color        = style.Colors[i];
        if (color.w < setup.alphaThreshold || colorId == ImGuiCol_FrameBg || colorId == ImGuiCol_WindowBg || colorId == ImGuiCol_ChildBg)
        {
            color.w *= setup.alphaThreshold;
        }
    }
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
