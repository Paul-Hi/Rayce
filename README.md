# Rayce

A future pathtracer that supports Vulkan hardware ray tracing.

## Dependencies

The Vulkan SDK must be installed. To compile slang shaders NVIDIAS slang compiler has to be installed.
Everything else is loaded via CMake.
To build the documentation a Doxygen installation is also required.

We use:

* Eigen for math
* glfw for window creation
* imgui for the graphical user interface
* googletest for testing
* Doxygen to generate the documentation
* stb_image to load image files
* TinyParser-Mitsuba to parse mitsuba scene xmls
* TinyXML-2 for TinyParser-Mitsuba to work
* miniply to load *.ply files