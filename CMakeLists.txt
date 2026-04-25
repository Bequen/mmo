cmake_minimum_required(VERSION 3.26)
project(towards)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/CompilerOptions.cmake)
include(cmake/Dependencies.cmake)

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

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src/
        ${CMAKE_SOURCE_DIR}/external/implot/
        ${JoltPhysics_SOURCE_DIR}/..
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        tw::network
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

include(cmake/Modules.cmake)
