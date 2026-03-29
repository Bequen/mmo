cmake_minimum_required(VERSION 3.26)
project(server_gui)


file(GLOB CXX_FILES
    src/*.cpp
)

add_executable(${PROJECT_NAME} ${CXX_FILES})

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    ./src/
)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
    mmo::network
    loft::common
    loft::base
    loft_window
    loft::render_graph
    tw::protocol
    towards
    mmo::gui
    imgui::imgui
    imgui::plot
    glm::glm
)
