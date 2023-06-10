import os, subprocess

def runSubprocessVerbose(args):
    print(*args)
    process = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)
    print(process.stdout.decode('utf8'))
    return process.returncode

def runGitSubmoduleCommands():
    result = runSubprocessVerbose(['git', 'submodule', 'init'])
    if result != 0:
        return False
    result = runSubprocessVerbose(['git', 'submodule', 'update', '--remote', '--merge'])
    if result != 0:
        return False
    return True

def createDependencyFiles():
    os.chdir('dependencies')

    # imgui CMake
    f = open('./imgui/CMakeLists.txt','w+')
    f.write('project(imgui)\r\n')
    f.write('add_library(imgui imgui.cpp imgui_demo.cpp imgui_draw.cpp imgui_widgets.cpp imgui_tables.cpp imgui.h imconfig.h imgui_internal.h imstb_rectpack.h imstb_textedit.h imstb_truetype.h backends/imgui_impl_glfw.h backends/imgui_impl_glfw.cpp backends/imgui_impl_vulkan.h backends/imgui_impl_vulkan.cpp)\r\n')
    f.write('target_include_directories(imgui SYSTEM PUBLIC . ../glfw/include ${Vulkan_INCLUDE_DIRS})\r\n')
    f.close()

    # tracy CMake
    f = open('./tracy/CMakeLists.txt','w+')
    f.write('project(tracy)\r\n')
    f.write('add_library(tracy TracyClient.cpp TracyVulkan.hpp)\r\n')
    f.write('target_link_libraries(tracy PUBLIC $<$<BOOL:${WIN32}>:wsock32> $<$<BOOL:${WIN32}>:ws2_32> $<$<BOOL:${WIN32}>:dbghelp>)\r\n')
    f.write('target_include_directories(tracy SYSTEM PUBLIC .)\r\n')
    f.write('target_compile_definitions(tracy PUBLIC $<$<CONFIG:Release>:$<$<BOOL:${RAYCE_PROFILE}>:TRACY_ENABLE>> $<$<BOOL:${WIN32}>:WINVER=0x0601 _WIN32_WINNT=0x0601 _WINSOCKAPI_>)\r\n')
    f.close()

    os.chdir('..')
    return True

def main():
    if not runGitSubmoduleCommands():
        print('Failed initializing git submodules.')
        os._exit(1)
    if not createDependencyFiles():
        print('Failed creating generated files.')
        os._exit(1)

    os._exit(0)

if __name__== '__main__':
   main()
