# Rayce

A future pathtracer that supports Vulkan hardware ray tracing.

## Dependencies

The Vulkan SDK must be installed. Everything else is loaded via CMake.
To build the documentation a Doxygen installation is also required.

We use:

* Eigen for math
* glfw for window creation
* imgui for the graphical user interface
* googletest for testing
* Doxygen to generate the documentation
* stb_image to load image files
* TinyParser-Mitsuba to parse mitsuba scene xmls