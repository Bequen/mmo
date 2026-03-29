cmake_minimum_required(VERSION 3.26)
project(towards)

enable_testing()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
endif()

set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld")
set(CMAKE_SHARED_LINKER_FLAGS "-fuse-ld=lld")
set(CMAKE_MODULE_LINKER_FLAGS "-fuse-ld=lld")
set(EXPECTED_BUILD_TESTS OFF CACHE BOOL "Disable expected tests" FORCE)

add_definitions(-DIMGUI_IMPL_VULKAN_USE_VOLK)
add_definitions(-DSPDLOG_COMPILED_LIB)
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
add_definitions(-DTRACY_ENABLE)

set(LFT_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)

find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

set(FASTNOISE2_NOISETOOL OFF CACHE BOOL "Build Noise Tool" FORCE) #example if don't need the graph tool

add_subdirectory(./external/loft)
add_subdirectory(./external/imgui)
add_subdirectory(./external/implot)
# add_subdirectory(./external/FastNoise2/)
#
option(TRACY_ENABLE "" ON)

# External libraries
include(FetchContent)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Distribution")

FetchContent_Declare(
         glm
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG        1.0.1
        SOURCE_DIR     ./external/glm/
		GIT_SHALLOW 1
)

FetchContent_Declare(
    entt
    GIT_REPOSITORY      https://github.com/skypjack/entt
    GIT_TAG             v3.15.0
    SOURCE_DIR          ./external/entt
    GIT_SHALLOW         1
)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.10.0
    FIND_PACKAGE_ARGS
)

FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY "https://github.com/jrouwe/JoltPhysics"
    GIT_TAG "v5.4.0"
    SOURCE_DIR ./external/jolt/
    SOURCE_SUBDIR "Build"
)

FetchContent_Declare(
    FastNoise2
    GIT_REPOSITORY "https://github.com/Auburn/FastNoise2"
    SOURCE_DIR ./external/FastNoise2/
)


FetchContent_Declare(
    spdlog
    GIT_REPOSITORY "https://github.com/gabime/spdlog"
    GIT_TAG "v1.16.0"
    GIT_SHALLOW         1
)

FetchContent_Declare(
    expected
    GIT_REPOSITORY https://github.com/TartanLlama/expected
    GIT_SHALLOW 1
)

FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glm entt Catch2 JoltPhysics spdlog expected tracy)

find_package(Protobuf CONFIG REQUIRED)

add_subdirectory(./modules/network/)
add_subdirectory(./modules/protocol/)
add_subdirectory(./modules/server/)
add_subdirectory(./modules/client/)
add_subdirectory(./modules/mock_client/)
add_subdirectory(./modules/gui/)
add_subdirectory(./modules/server_gui/)


file(GLOB FILES
    src/runtime/*.cpp
    src/world/*.cpp
    src/draw/*.cpp
    src/draw/RenderPasses/*.cpp
    src/debug/tools/*.cpp
    src/io/*.cpp
)

file(GLOB HEADERS
    src/runtime/*.hpp
    src/world/*.hpp
    src/draw/*.hpp
    src/debug/tools/*.hpp
    src/io/*.hpp
)

add_library(${PROJECT_NAME} OBJECT ${FILES})
add_dependencies(${PROJECT_NAME} imgui imgui::plot tw_shaders)
target_sources(${PROJECT_NAME}
    PUBLIC FILE_SET HEADERS
    BASE_DIRS src
    FILES ${HEADERS}
)

target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ${PROJECT_SOURCE_DIR}/src/
        ./external/implot/
        ${JoltPhysics_SOURCE_DIR}/..
)

target_link_libraries(
		${PROJECT_NAME}

        PUBLIC
        mmo::network
        tw::protocol

        loft::common
		loft::base
		loft::render_graph

        glm::glm
        EnTT::EnTT
        Jolt
        tl::expected
        Tracy::TracyClient
        pqxx
        pq
)
