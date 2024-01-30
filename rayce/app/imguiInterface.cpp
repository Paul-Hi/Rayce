/// @file      imguiInterface.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2024
/// @copyright Apache License 2.0

#include <app/imguiImpl.hpp>
#include <app/imguiInterface.hpp>
#include <imstb_truetype.h>
#include <vulkan/commandPool.hpp>
#include <vulkan/device.hpp>
#include <vulkan/framebuffer.hpp>
#include <vulkan/immediateSubmit.hpp>
#include <vulkan/instance.hpp>
#include <vulkan/renderPass.hpp>
#include <vulkan/swapchain.hpp>
#include <vulkan/window.hpp>

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

    vec2 scaleFactorFont = static_cast<Window::WindowData*>(glfwGetWindowUserPointer(nativeWindowHandle))->contentScale;

    setupImGuiStyle(scaleFactorFont.maxCoeff());

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

void ImguiInterface::begin(const std::unique_ptr<Window>& window)
{
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();

    // Dockspace
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    static ImGuiDockNodeFlags dockNodeFlags = ImGuiDockNodeFlags_PassthruCentralNode;

    static ImGuiWindowFlags dockingWindowFlags;
    dockingWindowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    dockingWindowFlags |= ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, dockingWindowFlags);
    ImGui::PopStyleVar(3);

    // Real Dockspace
    ImGuiID dockspaceId = ImGui::GetID("RayceDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockNodeFlags);

    // Menu bar
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
                window->close();
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void ImguiInterface::end(VkCommandBuffer commandBuffer, const std::unique_ptr<Framebuffer>& framebuffer, const std::vector<VkClearValue>& clearValues)
{
    ImGui::End(); // Dockspace End

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

void ImguiInterface::setupImGuiStyle(float scaleFactorFont)
{
    ImGuiIO& io = ImGui::GetIO();

    io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/Inter-VariableFont_slnt,wght.ttf", 18.0 * scaleFactorFont);

    static const ImWchar iconsRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig iconsConfig;
    iconsConfig.MergeMode  = true;
    iconsConfig.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAR, 16.0f * scaleFactorFont, &iconsConfig, iconsRanges);
    io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, 16.0f * scaleFactorFont, &iconsConfig, iconsRanges);

    io.ConfigWindowsMoveFromTitleBarOnly = true; // Only move from title bar

    // Colors
    ImVec4* colors = ImGui::GetStyle().Colors;

    colors[ImGuiCol_Text]                  = theme::text;
    colors[ImGuiCol_TextDisabled]          = theme::brighten;
    colors[ImGuiCol_WindowBg]              = theme::titlebar;        // Background of normal windows
    colors[ImGuiCol_ChildBg]               = theme::background;      // Background of child windows
    colors[ImGuiCol_PopupBg]               = theme::backgroundPopup; // Background of popups, menus, tooltips windows
    colors[ImGuiCol_Border]                = theme::backgroundDark;
    colors[ImGuiCol_BorderShadow]          = theme::black;
    colors[ImGuiCol_FrameBg]               = theme::backgroundProperty; // Background of checkbox, radio button, plot, slider, text input
    colors[ImGuiCol_FrameBgHovered]        = theme::backgroundProperty;
    colors[ImGuiCol_FrameBgActive]         = theme::backgroundProperty;
    colors[ImGuiCol_TitleBg]               = theme::titlebar;
    colors[ImGuiCol_TitleBgActive]         = theme::titlebar;
    colors[ImGuiCol_TitleBgCollapsed]      = theme::titlebarCollapsed;
    colors[ImGuiCol_MenuBarBg]             = theme::black;
    colors[ImGuiCol_ScrollbarBg]           = theme::black;
    colors[ImGuiCol_ScrollbarGrab]         = theme::backgroundGrab;
    colors[ImGuiCol_ScrollbarGrabHovered]  = theme::backgroundDark;
    colors[ImGuiCol_ScrollbarGrabActive]   = theme::activeGrab;
    colors[ImGuiCol_CheckMark]             = theme::highlight;
    colors[ImGuiCol_SliderGrab]            = theme::backgroundGrab;
    colors[ImGuiCol_SliderGrabActive]      = theme::activeGrab;
    colors[ImGuiCol_Button]                = theme::backgroundGrab;
    colors[ImGuiCol_ButtonHovered]         = theme::backgroundPopup;
    colors[ImGuiCol_ButtonActive]          = theme::accentClick;
    colors[ImGuiCol_Header]                = theme::header; // Header* colors are used for CollapsingHeader, TreeNode, Selectable, MenuItem
    colors[ImGuiCol_HeaderHovered]         = theme::header;
    colors[ImGuiCol_HeaderActive]          = theme::header;
    colors[ImGuiCol_Separator]             = theme::backgroundPopup;
    colors[ImGuiCol_SeparatorHovered]      = theme::backgroundDark;
    colors[ImGuiCol_SeparatorActive]       = theme::highlight;
    colors[ImGuiCol_ResizeGrip]            = theme::backgroundGrab; // Resize grip in lower-right and lower-left corners of windows.
    colors[ImGuiCol_ResizeGripHovered]     = theme::backgroundDark;
    colors[ImGuiCol_ResizeGripActive]      = theme::activeGrab;
    colors[ImGuiCol_Tab]                   = theme::titlebar; // TabItem in a TabBar
    colors[ImGuiCol_TabHovered]            = theme::titlebar;
    colors[ImGuiCol_TabActive]             = theme::titlebar;
    colors[ImGuiCol_TabUnfocused]          = theme::titlebarCollapsed;
    colors[ImGuiCol_TabUnfocusedActive]    = theme::titlebar;
    colors[ImGuiCol_DockingPreview]        = theme::brighten;   // Preview overlay color when about to docking something
    colors[ImGuiCol_DockingEmptyBg]        = theme::background; // Background color for empty node (e.g. CentralNode with no window docked into it)
    colors[ImGuiCol_PlotLines]             = theme::text;
    colors[ImGuiCol_PlotLinesHovered]      = theme::text;
    colors[ImGuiCol_PlotHistogram]         = theme::text;
    colors[ImGuiCol_PlotHistogramHovered]  = theme::text;
    colors[ImGuiCol_TableHeaderBg]         = theme::header;         // Table header background
    colors[ImGuiCol_TableBorderStrong]     = theme::backgroundDark; // Table outer and header borders (prefer using Alpha=1.0 here)
    colors[ImGuiCol_TableBorderLight]      = theme::backgroundDark; // Table inner borders (prefer using Alpha=1.0 here)
    colors[ImGuiCol_TableRowBg]            = theme::backgroundDark; // Table row background (even rows)
    colors[ImGuiCol_TableRowBgAlt]         = theme::background;     // Table row background (odd rows)
    colors[ImGuiCol_TextSelectedBg]        = theme::brighten;
    colors[ImGuiCol_DragDropTarget]        = theme::highlight; // Rectangle highlighting a drop target
    colors[ImGuiCol_NavHighlight]          = theme::highlight; // Gamepad/keyboard: current highlighted item
    colors[ImGuiCol_NavWindowingHighlight] = theme::highlight; // Highlight window when using CTRL+TAB
    colors[ImGuiCol_NavWindowingDimBg]     = theme::darken;    // Darken/colorize entire screen behind the CTRL+TAB window list, when active
    colors[ImGuiCol_ModalWindowDimBg]      = theme::darken;    // Darken/colorize entire screen behind a modal window, when one is active

    // Style
    ImGuiStyle& style      = ImGui::GetStyle();
    style.FrameRounding    = 3.5f;
    style.WindowRounding   = 0.0f;
    style.ChildBorderSize  = 1.0f;
    style.FrameBorderSize  = 1.0f;
    style.PopupBorderSize  = 1.0f;
    style.WindowBorderSize = 0.0f;
    style.IndentSpacing    = 11.0f;
    style.Alpha            = 1.0f;
    style.DisabledAlpha    = 0.5f;
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
