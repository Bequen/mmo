cmake_minimum_required(VERSION 3.26)
project(mmo_client)

add_subdirectory(shaders)

file(GLOB FILES
    src/*.cpp
    src/world/*.cpp
    src/io/*.cpp
    src/draw/*.cpp
    src/draw/RenderPasses/*.cpp
    src/debug/*.cpp
    src/debug/tools/*.cpp
)

add_executable(${PROJECT_NAME} ${FILES})
add_dependencies(tw_shaders imgui::plot imgui::plot)

target_link_libraries(${PROJECT_NAME} 
        PUBLIC 
        towards
        mmo::network
        loft::common
		loft::base
		loft_window
		loft::render_graph
        imgui::imgui
        imgui::plot
        glm::glm
        EnTT::EnTT
        FastNoise
        Jolt
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${PROJECT_SOURCE_DIR}/src/
        ${JoltPhysics_SOURCE_DIR}/..
)
