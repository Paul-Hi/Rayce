/// @file      rayceGui.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <rayce.hpp>

class RayceGui : public rayce::RayceApp
{
  public:
    RayceGui(const rayce::RayceOptions& options);

    bool onInitialize() override;
    bool onShutdown() override;
    void onUpdate() override;
    void onRender() override;
    void onImGuiRender() override;
};
