# Rayce

A future pathtracer that supports Vulkan hardware ray tracing and can load Universal Scene Description files.

## Prerequisites

The Vulkan SDK and USD must be installed. Everything else is loaded via CMake.
To build the documentation a Doxygen installation is also required.

If CMake does not find USD, PXR_DIR must be set to the root folder of the installation.

Keep in mind that that the compilation of USD and Rayce has to be done with the same compiler.

Other possibly necessary variables are:

| Variable          | Meaning                            | Value                                                        |
|-------------------|------------------------------------|--------------------------------------------------------------|
| **PXR_DIR**       | Root directory of USD installation |                                                              |
| **MATERIALX_DIR** | sometimes required as well         | PXR_DIR/lib/cmake/MaterialX                                  |
