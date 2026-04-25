set(EXPECTED_BUILD_TESTS  OFF CACHE BOOL "Disable tl::expected tests"    FORCE)
set(BUILD_EXAMPLES        OFF CACHE BOOL "Disable dependency examples"   FORCE)
set(LFT_ENABLE_EXAMPLES   OFF CACHE BOOL "Disable loft examples"         FORCE)
set(FASTNOISE2_NOISETOOL  OFF CACHE BOOL "Disable FastNoise2 graph tool" FORCE)

find_package(Vulkan   REQUIRED)
find_package(SDL2     REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

include(FetchContent)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Distribution")

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG        1.0.1
    SOURCE_DIR     ${CMAKE_SOURCE_DIR}/external/glm/
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt
    GIT_TAG        v3.15.0
    SOURCE_DIR     ${CMAKE_SOURCE_DIR}/external/entt
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.10.0
    FIND_PACKAGE_ARGS
)

FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics
    GIT_TAG        v5.4.0
    SOURCE_DIR     ${CMAKE_SOURCE_DIR}/external/jolt/
    SOURCE_SUBDIR  Build
)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG        v1.16.0
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    expected
    GIT_REPOSITORY https://github.com/TartanLlama/expected
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

# FetchContent_Declare(
#     Quadtree
#     GIT_REPOSITORY https://github.com/pvigier/Quadtree.git
#     GIT_SHALLOW    TRUE
# )

FetchContent_MakeAvailable(glm entt Catch2 JoltPhysics spdlog expected tracy)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/loft)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/imgui)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/implot)
