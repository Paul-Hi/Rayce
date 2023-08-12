/// @file      input.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef RAYCE_INPUT_HPP
#define RAYCE_INPUT_HPP

#include <core/Signal.hpp>
#include <core/inputCodes.hpp>

namespace rayce
{
    class RAYCE_API_EXPORT Input
    {
    public:
        Input();
        ~Input() = default;

        /// @brief Retrieves the current state of a specific key.
        /// @param[in] key The @a EKeyCode of the key to query the state for.
        /// @return The @a EInputAction representing the state of the key.
        EInputAction getKey(EKeyCode key);

        /// @brief Retrieves the current state of a specific mouse button.
        /// @param[in] button The @a EMouseButton to query the state for.
        /// @return The @a EInputAction representing the state of the mouse button.
        EInputAction getMouseButton(EMouseButton button);

        /// @brief Retrieves the currently activated EModifiers.
        /// @details These are special keys that are relevant if pressed at the same time with other actions.
        /// @return The @a EModifier including all active EModifiers.
        EModifier getModifiers();

        /// @brief Retrieves the current cursor position.
        /// @return The current position of the cursor.
        dvec2 getCursorPosition();

        /// @brief Retrieves the current scroll offsets.
        /// @return The scroll offset in x and y direction.
        dvec2 getScrollOffset();

        //
        //
        //

        /// @brief Registers a callback function getting called on @a Window position change.
        /// @param[in] callback Function to callback.
        void registerWindowPositionCallback(WindowPositionCallback callback);

        /// @brief Registers a callback function getting called on @a Window resize.
        /// @param[in] callback Function to callback.
        void registerWindowResizeCallback(WindowResizeCallback callback);

        /// @brief Registers a callback function getting called on @a Window close.
        /// @param[in] callback Function to callback.
        void registerWindowCloseCallback(WindowCloseCallback callback);

        /// @brief Registers a callback function getting called on @a Window refresh.
        /// @param[in] callback Function to callback.
        void registerWindowRefreshCallback(WindowRefreshCallback callback);

        /// @brief Registers a callback function getting called on @a Window focus change.
        /// @param[in] callback Function to callback.
        void registerWindowFocusCallback(WindowFocusCallback callback);

        /// @brief Registers a callback function getting called on @a Window iconification change.
        /// @param[in] callback Function to callback.
        void registerWindowIconifyCallback(WindowIconifyCallback callback);

        /// @brief Registers a callback function getting called on @a Window maximization change.
        /// @param[in] callback Function to callback.
        void registerWindowMaximizeCallback(WindowMaximizeCallback callback);

        /// @brief Registers a callback function getting called on @a Window framebuffer resize.
        /// @param[in] callback Function to callback.
        void registerWindowFramebufferResizeCallback(WindowFramebufferResizeCallback callback);

        /// @brief Registers a callback function getting called on @a Window content scale change.
        /// @param[in] callback Function to callback.
        void registerWindowContentScaleCallback(WindowContentScaleCallback callback);

        /// @brief Registers a callback function getting called on mouse button events.
        /// @param[in] callback Function to callback.
        void registerMouseButtonCallback(MouseButtonCallback callback);

        /// @brief Registers a callback function getting called on cursor position change.
        /// @param[in] callback Function to callback.
        void registerCursorPositionCallback(CursorPositionCallback callback);

        /// @brief Registers a callback function getting called on cursor entering or exiting the @a Window.
        /// @param[in] callback Function to callback.
        void registerCursorEnterCallback(CursorEnterCallback callback);

        /// @brief Registers a callback function getting called on scroll events.
        /// @param[in] callback Function to callback.
        void registerScrollCallback(ScrollCallback callback);

        /// @brief Registers a callback function getting called on key events.
        /// @param[in] callback Function to callback.
        void registerKeyCallback(KeyCallback callback);

        /// @brief Registers a callback function getting called on drop events.
        /// @param[in] callback Function to callback.
        void registerDropCallback(DropCallback callback);

        /// @brief Signals window position change.
        /// @param[in] xPosition The new upper-left corner x position in screen coordinates.
        /// @param[in] yPosition he new upper-left corner y position in screen coordinates.
        void onWindowPosition(int32 xPosition, int32 yPosition);

        /// @brief Signals window resize event.
        /// @param[in] width The new width of the window in screen coordinates.
        /// @param[in] height The new height of the window in screen coordinates.
        void onWindowResize(int32 width, int32 height);

        /// @brief Signals window close event.
        void onWindowClose();

        /// @brief Signals window refresh event.
        void onWindowRefresh();

        /// @brief Signals window focus change.
        /// @param[in] focused True if the window was given input focus, or false if it lost it.
        void onWindowFocus(bool focused);

        /// @brief Signals window iconify event.
        /// @param[in] iconified True if the window was iconified, or false if it was restored.
        void onWindowIconify(bool iconified);

        /// @brief Signals window maximize event.
        /// @param[in] maximized True if the window was maximized, or false if it was restored.
        void onWindowMaximize(bool maximized);

        /// @brief Signals window framebuffer resize event.
        /// @param[in] width The new width of the window framebuffer in pixels.
        /// @param[in] height The new height of the window framebuffer in pixels.
        void onWindowFramebufferResize(int32 width, int32 height);

        /// @brief Signals window content scale change.
        /// @param[in] xScale The new x-axis content scale of the window.
        /// @param[in] yScale The new y-axis content scale of the window.
        void onWindowContentScale(float xScale, float yScale);

        /// @brief Signals mouse button events.
        /// @param[in] button The @a EMouseButton that was pressed or released.
        /// @param[in] action Can be EInputAction::press or EInputAction::release.
        /// @param[in] mods Bit field describing which modifier keys were held down.
        void onInputMouseButton(EMouseButton button, EInputAction action, EModifier mods);

        /// @brief Signals cursor position changes.
        /// @param[in] xPosition The new cursor x-coordinate, relative to the left edge of the content area.
        /// @param[in] yPosition The new cursor y-coordinate, relative to the top edge of the content area.
        void onInputCursorPosition(double xPosition, double yPosition);

        /// @brief Signals cursor enter events.
        /// @param[in] entered True if the cursor entered the window's content area, false if it left it.
        void onInputCursorEnter(bool entered);

        /// @brief Signals scroll events.
        /// @param[in] xOffset The scroll offset along the x-axis.
        /// @param[in] yOffset The scroll offset along the y-axis.
        void onInputScroll(double xOffset, double yOffset);

        /// @brief Signals key events.
        /// @param[in] key The @a EKeyCode that was pressed or released.
        /// @param[in] action Can be EInputAction::press or EInputAction::release.
        /// @param[in] mods Bit field describing which modifier keys were held down.
        void onInputKey(EKeyCode key, EInputAction action, EModifier mods);

        /// @brief Signals drop events.
        /// @param[in] pathCount The number of paths.
        /// @param[in] paths The file and/or directory path names.
        void onInputDrop(int32 pathCount, const char** paths);

    private:
        /// @brief Structure containing the input state that can be polled directly.
        struct InputState
        {
            /// @brief Mouse button map.
            std::array<EInputAction, static_cast<ptr_size>(EMouseButton::count)> mouseButtons;
            /// @brief Keymap.
            std::array<EInputAction, static_cast<ptr_size>(EKeyCode::count)> keys;
            /// @brief Active EModifiers.
            EModifier modifierField;
            /// @brief Current cursor position.
            dvec2 cursorPosition;
            /// @brief Current scroll offset.
            dvec2 scrollOffset;
        };

        /// @brief Current input state.
        InputState mInputState;

        struct
        {
            /// @brief Used @a Signal for window position changes.
            Signal<int32, int32> windowPosition;
            /// @brief Used @a Signal for window resize.
            Signal<int32, int32> windowResize;
            /// @brief Used @a Signal for window close.
            Signal<> windowClose;
            /// @brief Used @a Signal for window refresh.
            Signal<> windowRefresh;
            /// @brief Used @a Signal for window focus changes.
            Signal<bool> windowFocus;
            /// @brief Used @a Signal for window iconification.
            Signal<bool> windowIconify;
            /// @brief Used @a Signal for window maximization.
            Signal<bool> windowMaximize;
            /// @brief Used @a Signal for window framebuffer resize.
            Signal<int32, int32> windowFramebufferResize;
            /// @brief Used @a Signal for window content scale.
            Signal<float, float> windowContentScale;
            /// @brief Used @a Signal for mouse button input.
            Signal<EMouseButton, EInputAction, EModifier> inputMouseButton;
            /// @brief Used @a Signal for cursor movement.
            Signal<double, double> inputCursorPosition;
            /// @brief Used @a Signal for curso enter status changes.
            Signal<bool> inputCursorEnter;
            /// @brief Used @a Signal for scrolling.
            Signal<double, double> inputScroll;
            /// @brief Used @a Signal for key input.
            Signal<EKeyCode, EInputAction, EModifier> inputKey;
            /// @brief Used @a Signal for drop events.
            Signal<int32, const char**> inputDrop;
        } mSignals; ///< Signals used for connecting functions and calling them on events.
    };
} // namespace rayce

#endif // RAYCE_INPUT_HPP
