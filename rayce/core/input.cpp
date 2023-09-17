/// @file      input.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <core/input.hpp>
#include <imgui.h>

using namespace rayce;

Input::Input()
{
    mInputState.keys.fill(EInputAction::release);
    mInputState.mouseButtons.fill(EInputAction::release);
    mInputState.modifierField  = EModifier::none;
    mInputState.cursorPosition = dvec2(0.0f, 0.0f);
    mInputState.scrollOffset   = dvec2(0.0f, 0.0f);
    mInputState.hovered        = true;

    mSignals.inputKey.connect([this](EKeyCode key, EInputAction action, EModifier mods)
                              {
        if(static_cast<ptr_size>(key) >= mInputState.keys.size())
            return;
        mInputState.keys[static_cast<int32>(key)] = action;
        mInputState.modifierField &= mods; });

    mSignals.inputMouseButton.connect([this](EMouseButton button, EInputAction action, EModifier mods)
                                      {
        mInputState.mouseButtons[static_cast<int32>(button)] = action;
        mInputState.modifierField &= mods; });

    mSignals.inputCursorPosition.connect([this](double x_position, double y_position)
                                         {
        mInputState.cursorPosition.x() = x_position;
        mInputState.cursorPosition.y() = y_position; });

    mSignals.inputScroll.connect([this](double x_offset, double y_offset)
                                 {
        mInputState.scrollOffset.x() = x_offset;
        mInputState.scrollOffset.y() = y_offset; });

    mSignals.inputCursorEnter.connect([this](bool entered)
                                      {
        mInputState.hovered = entered;
        if(!entered)
        {
            clearInputState();
        } });
}

EInputAction Input::getKey(EKeyCode key)
{
    return mInputState.keys[static_cast<int32>(key)];
}

EInputAction Input::getMouseButton(EMouseButton button)
{
    return mInputState.mouseButtons[static_cast<int32>(button)];
}

EModifier Input::getModifiers()
{
    return mInputState.modifierField;
}

dvec2 Input::getCursorPosition()
{
    return mInputState.cursorPosition;
}

dvec2 Input::getScrollOffset()
{
    return mInputState.scrollOffset;
}

bool Input::isHovered()
{
    return mInputState.hovered;
}

void Input::clearInputState()
{
    mInputState.keys.fill(EInputAction::release);
    mInputState.mouseButtons.fill(EInputAction::release);
    mInputState.modifierField  = EModifier::none;
    mInputState.cursorPosition = dvec2(0.0f, 0.0f);
    mInputState.scrollOffset   = dvec2(0.0f, 0.0f);
}

void Input::registerWindowPositionCallback(WindowPositionCallback callback)
{
    mSignals.windowPosition.connect(callback);
}

void Input::registerWindowResizeCallback(WindowResizeCallback callback)
{
    mSignals.windowResize.connect(callback);
}

void Input::registerWindowCloseCallback(WindowCloseCallback callback)
{
    mSignals.windowClose.connect(callback);
}

void Input::registerWindowRefreshCallback(WindowRefreshCallback callback)
{
    mSignals.windowRefresh.connect(callback);
}

void Input::registerWindowFocusCallback(WindowFocusCallback callback)
{
    mSignals.windowFocus.connect(callback);
}

void Input::registerWindowIconifyCallback(WindowIconifyCallback callback)
{
    mSignals.windowIconify.connect(callback);
}

void Input::registerWindowMaximizeCallback(WindowMaximizeCallback callback)
{
    mSignals.windowMaximize.connect(callback);
}

void Input::registerWindowFramebufferResizeCallback(WindowFramebufferResizeCallback callback)
{
    mSignals.windowFramebufferResize.connect(callback);
}

void Input::registerWindowContentScaleCallback(WindowContentScaleCallback callback)
{
    mSignals.windowContentScale.connect(callback);
}

void Input::registerMouseButtonCallback(MouseButtonCallback callback)
{
    mSignals.inputMouseButton.connect(callback);
}

void Input::registerCursorPositionCallback(CursorPositionCallback callback)
{
    mSignals.inputCursorPosition.connect(callback);
}

void Input::registerCursorEnterCallback(CursorEnterCallback callback)
{
    mSignals.inputCursorEnter.connect(callback);
}

void Input::registerScrollCallback(ScrollCallback callback)
{
    mSignals.inputScroll.connect(callback);
}

void Input::registerKeyCallback(KeyCallback callback)
{
    mSignals.inputKey.connect(callback);
}

void Input::registerDropCallback(DropCallback callback)
{
    mSignals.inputDrop.connect(callback);
}

void Input::onWindowPosition(int32 xPosition, int32 yPosition)
{
    mSignals.windowPosition(xPosition, yPosition);
}

void Input::onWindowResize(int32 width, int32 height)
{
    mSignals.windowResize(width, height);
}

void Input::onWindowClose()
{
    mSignals.windowClose();
}

void Input::onWindowRefresh()
{
    mSignals.windowRefresh();
}

void Input::onWindowFocus(bool focused)
{
    mSignals.windowFocus(focused);
}

void Input::onWindowIconify(bool iconified)
{
    mSignals.windowIconify(iconified);
}

void Input::onWindowMaximize(bool maximized)
{
    mSignals.windowMaximize(maximized);
}

void Input::onWindowFramebufferResize(int32 width, int32 height)
{
    mSignals.windowFramebufferResize(width, height);
}

void Input::onWindowContentScale(float xScale, float yScale)
{
    mSignals.windowContentScale(xScale, yScale);
}

void Input::onInputMouseButton(EMouseButton button, EInputAction action, EModifier mods)
{
    ImGuiIO io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        return;
    }
    mSignals.inputMouseButton(button, action, mods);
}

void Input::onInputCursorPosition(double xPosition, double yPosition)
{
    mSignals.inputCursorPosition(xPosition, yPosition);
}

void Input::onInputCursorEnter(bool entered)
{
    mSignals.inputCursorEnter(entered);
}

void Input::onInputScroll(double xOffset, double yOffset)
{
    mSignals.inputScroll(xOffset, yOffset);
}

void Input::onInputKey(EKeyCode key, EInputAction action, EModifier mods)
{
    ImGuiIO io = ImGui::GetIO();
    if (io.WantCaptureKeyboard)
    {
        return;
    }
    mSignals.inputKey(key, action, mods);
}

void Input::onInputDrop(int32 pathCount, const char** paths)
{
    mSignals.inputDrop(pathCount, paths);
}
