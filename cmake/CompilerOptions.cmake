if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
endif()

add_compile_options(-fno-omit-frame-pointer)
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR NOT CMAKE_BUILD_TYPE)
    add_compile_options(-g)
endif()

option(USE_MOLD_LINKER "Use mold as the linker" ON)
if(USE_MOLD_LINKER)
    add_link_options(-fuse-ld=mold)
endif()

option(TRACY_ENABLE "Enable Tracy profiler" ON)

add_compile_definitions(
    IMGUI_IMPL_VULKAN_USE_VOLK
    SPDLOG_COMPILED_LIB
    GLM_ENABLE_EXPERIMENTAL
    TRACY_ENABLE
    TRACY_CALLSTACK=1
)
