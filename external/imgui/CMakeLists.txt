project(imgui)

file(GLOB FILES
    ./*.cpp
    ./backends/imgui_impl_vulkan.cpp
    ./backends/imgui_impl_sdl2.cpp
)

find_package(SDL2 QUIET)
find_package(Vulkan REQUIRED)

add_library(${PROJECT_NAME} OBJECT ${FILES})
add_library(imgui::imgui ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${PROJECT_SOURCE_DIR}/backends
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        volk::volk
        ${SDL2_LIBRARIES}
)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${volk_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
)
