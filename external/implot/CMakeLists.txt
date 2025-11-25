project(imgui_plot)

file(GLOB FILES
    ${PROJECT_SOURCE_DIR}/*.cpp
    ./*.h
)

file(GLOB HEADERS
    *.h
)

add_library(${PROJECT_NAME} ${FILES})
add_library(imgui::plot ALIAS ${PROJECT_NAME})
add_dependencies(${PROJECT_NAME} imgui::imgui)

# target_include_directories(${PROJECT_NAME} INTERFACE 
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
# )

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./
)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        volk::volk
        ${SDL2_LIBRARIES}
        imgui::imgui
)
#
# target_include_directories(${PROJECT_NAME}
#         PRIVATE
#         ${volk_INCLUDE_DIRS}
#         ${SDL2_INCLUDE_DIRS}
# )
