/// @file      simpleGUI.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <app/rayce.hpp>

class SimpleGUI : public rayce::RayceApp
{
  public:
    SimpleGUI(const rayce::RayceOptions& options);

    bool onInitialize() override;
    bool onShutdown() override;
    void onFrameDraw() override;
    void onUpdate() override;
    void onRender(VkCommandBuffer commandBuffer, const rayce::uint32 imageIndex) override;
    void onImGuiRender(VkCommandBuffer commandBuffer, const rayce::uint32 imageIndex) override;
};
