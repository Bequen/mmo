cmake_minimum_required(VERSION 3.26)

project(mmo_gui)

file(GLOB CXX_FILES
    src/*.cpp
)

file(GLOB HEADER_FILES
    include/*.hpp
)

add_library(${PROJECT_NAME} OBJECT ${CXX_FILES})
add_library(mmo::gui ALIAS ${PROJECT_NAME})

target_sources(
    ${PROJECT_NAME}
    PUBLIC FILE_SET HEADERS
    BASE_DIRS include
    FILES ${HEADER_FILES}
)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    ./include/
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    spdlog::spdlog
    loft::common
    loft::base
    loft::render_graph
    loft_window
    imgui::imgui
    imgui::plot
    glm::glm
)
