/// @file      window.cpp
/// @author    Paul Himmler
/// @version   0.01
/// @date      2023
/// @copyright Apache License 2.0

#include <core/input.hpp>
#include <vulkan/window.hpp>

using namespace rayce;

static void glfwErrorCallback(int errorCode, const char* description)
{
    RAYCE_LOG_ERROR("GLFW Error {0}: {1}", errorCode, description);
}

Window::Window(int32 width, int32 height, const str& name, const std::shared_ptr<Input> input)
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    mWindowData.width  = width;
    mWindowData.height = height;
    mWindowData.title  = name.c_str();
    mWindowData.pInput = input;

    mWindowData.pWindow = glfwCreateWindow(mWindowData.width, mWindowData.height, name.c_str(), nullptr, nullptr);

    RAYCE_CHECK_NOTNULL(mWindowData.pWindow, "Creating GLFW window failed!");

    glfwGetWindowPos(mWindowData.pWindow, &mWindowData.x, &mWindowData.y);
    glfwSetWindowUserPointer(mWindowData.pWindow, &mWindowData);

    {
        // Error callback
        glfwSetErrorCallback(&glfwErrorCallback);

        // Window position callback
        glfwSetWindowPosCallback(mWindowData.pWindow,
                                 [](GLFWwindow* window, int xPos, int yPos)
                                 {
                                     WindowData* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                                     data->x          = xPos;
                                     data->y          = yPos;
                                     if (data->pInput.expired())
                                     {
                                         RAYCE_LOG_WARN("Input not valid. Can not send events.");
                                         return;
                                     }
                                     data->pInput.lock()->onWindowPosition(xPos, yPos);
                                 });

        // Window size callback
        glfwSetWindowSizeCallback(mWindowData.pWindow,
                                  [](GLFWwindow* window, int w, int h)
                                  {
                                      WindowData* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                                      data->width      = w;
                                      data->height     = h;
                                      if (data->pInput.expired())
                                      {
                                          RAYCE_LOG_WARN("Input not valid. Can not send events.");
                                          return;
                                      }
                                      data->pInput.lock()->onWindowResize(w, h);
                                  });

        // Window close callback
        glfwSetWindowCloseCallback(mWindowData.pWindow,
                                   [](GLFWwindow* window)
                                   {
                                       WindowData* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                                       if (data->pInput.expired())
                                       {
                                           RAYCE_LOG_WARN("Input not valid. Can not send events.");
                                           return;
                                       }
                                       data->pInput.lock()->onWindowClose();
                                   });

        // Window refresh callback
        glfwSetWindowRefreshCallback(mWindowData.pWindow,
                                     [](GLFWwindow* window)
                                     {
                                         WindowData* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                                         if (data->pInput.expired())
                                         {
                                             RAYCE_LOG_WARN("Input not valid. Can not send events.");
                                             return;
                                         }
                                         data->pInput.lock()->onWindowRefresh();
                                     });

        // Window focus callback
        glfwSetWindowFocusCallback(mWindowData.pWindow,
                                   [](GLFWwindow* window, int focused)
                                   {
                                       WindowData* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                                       if (data->pInput.expired())
                                       {
                                           RAYCE_LOG_WARN("Input not valid. Can not send events.");
                                           return;
                                       }
                                       data->pInput.lock()->onWindowFocus(focused);
                                   });

        // Window iconify callback
        glfwSetWindowIconifyCallback(mWindowData.pWindow,
                                     [](GLFWwindow* window, int iconified)
                                     {
                                         WindowData* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                                         if (data->pInput.expired())
                                         {
                                             RAYCE_LOG_WARN("Input not valid. Can not send events.");
                                             return;
                                         }
                                         data->pInput.lock()->onWindowIconify(iconified);
                                     });

        // Window maximize callback
        glfwSetWindowMaximizeCallback(mWindowData.pWindow,
                                      [](GLFWwindow* window, int maximized)
                                      {
                                          WindowData* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                                          if (data->pInput.expired())
                                          {
                                              RAYCE_LOG_WARN("Input not valid. Can not send events.");
                                              return;
                                          }
                                          data->pInput.lock()->onWindowMaximize(maximized);
                                      });

        // Window framebuffer size callback
        glfwSetFramebufferSizeCallback(mWindowData.pWindow,
                                       [](GLFWwindow* window, int w, int h)
                                       {
                                           WindowData* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                                           if (data->pInput.expired())
                                           {
                                               RAYCE_LOG_WARN("Input not valid. Can not send events.");
                                               return;
                                           }
                                           data->pInput.lock()->onWindowFramebufferResize(w, h);
                                       });

        // Window content scale callback
        glfwSetWindowContentScaleCallback(mWindowData.pWindow,
                                          [](GLFWwindow* window, float x_scale, float y_scale)
                                          {
                                              WindowData* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                                              if (data->pInput.expired())
                                              {
                                                  RAYCE_LOG_WARN("Input not valid. Can not send events.");
                                                  return;
                                              }
                                              data->pInput.lock()->onWindowContentScale(x_scale, y_scale);
                                          });

        // Mouse button callback
        glfwSetMouseButtonCallback(mWindowData.pWindow,
                                   [](GLFWwindow* window, int button, int action, int mods)
                                   {
                                       WindowData* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                                       if (data->pInput.expired())
                                       {
                                           RAYCE_LOG_WARN("Input not valid. Can not send events.");
                                           return;
                                       }
                                       data->pInput.lock()->onInputMouseButton(static_cast<EMouseButton>(button), static_cast<EInputAction>(action), static_cast<EModifier>(mods));
                                   });

        // Cursor position callback
        glfwSetCursorPosCallback(mWindowData.pWindow,
                                 [](GLFWwindow* window, double x_pos, double y_pos)
                                 {
                                     WindowData* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                                     if (data->pInput.expired())
                                     {
                                         RAYCE_LOG_WARN("Input not valid. Can not send events.");
                                         return;
                                     }
                                     data->pInput.lock()->onInputCursorPosition(x_pos, y_pos);
                                 });

        // Cursor enter callback
        glfwSetCursorEnterCallback(mWindowData.pWindow,
                                   [](GLFWwindow* window, int entered)
                                   {
                                       WindowData* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                                       if (data->pInput.expired())
                                       {
                                           RAYCE_LOG_WARN("Input not valid. Can not send events.");
                                           return;
                                       }
                                       data->pInput.lock()->onInputCursorEnter(entered);
                                   });

        // Scroll callback
        glfwSetScrollCallback(mWindowData.pWindow,
                              [](GLFWwindow* window, double x_off, double y_off)
                              {
                                  WindowData* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                                  if (data->pInput.expired())
                                  {
                                      RAYCE_LOG_WARN("Input not valid. Can not send events.");
                                      return;
                                  }
                                  data->pInput.lock()->onInputScroll(x_off, y_off);
                              });

        // Key callback
        glfwSetKeyCallback(mWindowData.pWindow,
                           [](GLFWwindow* window, int key, int, int action, int mods)
                           {
                               WindowData* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                               if (data->pInput.expired())
                               {
                                   RAYCE_LOG_WARN("Input not valid. Can not send events.");
                                   return;
                               }
                               data->pInput.lock()->onInputKey(static_cast<EKeyCode>(key), static_cast<EInputAction>(action), static_cast<EModifier>(mods));
                           });

        // Drop callback
        glfwSetDropCallback(mWindowData.pWindow,
                            [](GLFWwindow* window, int path_count, const char** paths)
                            {
                                WindowData* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
                                if (data->pInput.expired())
                                {
                                    RAYCE_LOG_WARN("Input not valid. Can not send events.");
                                    return;
                                }
                                data->pInput.lock()->onInputDrop(path_count, paths);
                            });
    }

    RAYCE_LOG_INFO("Created window of size (%d, %d)!", mWindowData.width, mWindowData.height);
}

Window::~Window()
{
    if (mWindowData.pWindow)
    {
        glfwDestroyWindow(mWindowData.pWindow);
    }
    glfwTerminate();
}

std::vector<const char*> Window::getVulkanExtensions() const
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    return std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);
}
