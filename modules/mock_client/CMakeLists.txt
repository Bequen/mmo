project(tw_mock_client)

file(GLOB FILES
    src/*.cpp
)

add_executable(${PROJECT_NAME} ${FILES})

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        towards
        tw::network
        tw::protocol
        glm::glm
        EnTT::EnTT
        Jolt
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${PROJECT_SOURCE_DIR}/src/
        ${JoltPhysics_SOURCE_DIR}/..
)
