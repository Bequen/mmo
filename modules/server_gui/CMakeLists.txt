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
    tw::network
    loft::common
    loft::base
    loft_window
    loft::render_graph
    tw::protocol
    towards
    tw::gui
    imgui::imgui
    imgui::plot
    glm::glm
)
