/// @file      inputCodes.hpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#pragma once

#ifndef RAYCE_INPUT_CODES_HPP
#define RAYCE_INPUT_CODES_HPP

namespace rayce
{
    /// @brief Possible key codes.
    /// @details Actually glfw key codes for now. Just unknown is 0 instead of -1.
    enum class EKeyCode : uint32
    {
        keyUnknown      = 0,
        keySpace        = 32,
        keyApostrophe   = 39,
        keyComma        = 44,
        keyMinus        = 45,
        keyPeriod       = 46,
        keySlash        = 47,
        key0            = 48,
        key1            = 49,
        key2            = 50,
        key3            = 51,
        key4            = 52,
        key5            = 53,
        key6            = 54,
        key7            = 55,
        key8            = 56,
        key9            = 57,
        keySemicolon    = 59,
        keyEqual        = 61,
        keyA            = 65,
        keyB            = 66,
        keyC            = 67,
        keyD            = 68,
        keyE            = 69,
        keyF            = 70,
        keyG            = 71,
        keyH            = 72,
        keyI            = 73,
        keyJ            = 74,
        keyK            = 75,
        keyL            = 76,
        keyM            = 77,
        keyN            = 78,
        keyO            = 79,
        keyP            = 80,
        keyQ            = 81,
        keyR            = 82,
        keyS            = 83,
        keyT            = 84,
        keyU            = 85,
        keyV            = 86,
        keyW            = 87,
        keyX            = 88,
        keyY            = 89,
        keyZ            = 90,
        keyLeftBracket  = 91,
        keyBackslash    = 92,
        keyRightBracket = 93,
        keyGraveAccent  = 96,
        keyWorld1       = 161,
        keyWorld2       = 162,
        keyEscape       = 256,
        keyEnter        = 257,
        keyTab          = 258,
        keyBackspace    = 259,
        keyInsert       = 260,
        keyDelete       = 261,
        keyRight        = 262,
        keyLeft         = 263,
        keyDown         = 264,
        keyUp           = 265,
        keyPageUp       = 266,
        keyPageDown     = 267,
        keyHome         = 268,
        keyEnd          = 269,
        keyCapsLock     = 280,
        keyScrollLock   = 281,
        keyNumLock      = 282,
        keyPrintScreen  = 283,
        keyPause        = 284,
        keyF1           = 290,
        keyF2           = 291,
        keyF3           = 292,
        keyF4           = 293,
        keyF5           = 294,
        keyF6           = 295,
        keyF7           = 296,
        keyF8           = 297,
        keyF9           = 298,
        keyF10          = 299,
        keyF11          = 300,
        keyF12          = 301,
        keyF13          = 302,
        keyF14          = 303,
        keyF15          = 304,
        keyF16          = 305,
        keyF17          = 306,
        keyF18          = 307,
        keyF19          = 308,
        keyF20          = 309,
        keyF21          = 310,
        keyF22          = 311,
        keyF23          = 312,
        keyF24          = 313,
        keyF25          = 314,
        keyKp0          = 320,
        keyKp1          = 321,
        keyKp2          = 322,
        keyKp3          = 323,
        keyKp4          = 324,
        keyKp5          = 325,
        keyKp6          = 326,
        keyKp7          = 327,
        keyKp8          = 328,
        keyKp9          = 329,
        keyKpDecimal    = 330,
        keyKpDivide     = 331,
        keyKpMultiply   = 332,
        keyKpSubtract   = 333,
        keyKpAdd        = 334,
        keyKpEnter      = 335,
        keyKpEqual      = 336,
        keyLeftShift    = 340,
        keyLeftControl  = 341,
        keyLeftAlt      = 342,
        keyLeftSuper    = 343,
        keyRightShift   = 344,
        keyRightControl = 345,
        keyRightAlt     = 346,
        keyRightSuper   = 347,
        keyMenu         = 348,
        count
    };

    /// @brief Possible mouse button identifiers.
    /// @details Actually glfw mouse button codes for now.
    enum class EMouseButton : byte
    {
        mouseButton1      = 0,
        mouseButton2      = 1,
        mouseButton3      = 2,
        mouseButton4      = 3,
        mouseButton5      = 4,
        mouseButton6      = 5,
        mouseButton7      = 6,
        mouseButton8      = 7,
        mouseButtonLeft   = mouseButton1,
        mouseButtonRight  = mouseButton2,
        mouseButtonMiddle = mouseButton3,
        count             = 8
    };

    /// @brief Possible actions for input.
    /// @details Actually action from glfw for now.
    enum class EInputAction : byte
    {
        release = 0,
        press   = 1,
        repeat  = 2
    };

    /// @brief Possible modifier keys.
    /// @details Actually modifiers from glfw for now.
    enum class EModifier : byte
    {
        none             = 0,
        modifierShift    = 1 << 0,
        modifierControl  = 1 << 1,
        modifierAlt      = 1 << 2,
        modifierSuper    = 1 << 3,
        modifierCapsLock = 1 << 4,
        modifierNumLock  = 1 << 5
    };
    ENABLE_BITMASK_OPERATIONS(EModifier)

    using WindowPositionCallback          = std::function<void(int32 x_position, int32 y_position)>;
    using WindowResizeCallback            = std::function<void(int32 width, int32 height)>;
    using WindowCloseCallback             = std::function<void()>;
    using WindowRefreshCallback           = std::function<void()>;
    using WindowFocusCallback             = std::function<void(bool focused)>;
    using WindowIconifyCallback           = std::function<void(bool iconified)>;
    using WindowMaximizeCallback          = std::function<void(bool maximized)>;
    using WindowFramebufferResizeCallback = std::function<void(int32 width, int32 height)>;
    using WindowContentScaleCallback      = std::function<void(float x_scale, float y_scale)>;

    using MouseButtonCallback    = std::function<void(EMouseButton button, EInputAction action, EModifier mods)>;
    using CursorPositionCallback = std::function<void(double x_position, double y_position)>;
    using CursorEnterCallback    = std::function<void(bool entered)>;
    using ScrollCallback         = std::function<void(double x_offset, double y_offset)>;
    using KeyCallback            = std::function<void(EKeyCode key, EInputAction action, EModifier mods)>;
    using DropCallback           = std::function<void(int count, const char** paths)>;

} // namespace rayce

#endif // RAYCE_INPUT_CODES_HPP
