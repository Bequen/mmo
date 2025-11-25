cmake_minimum_required(VERSION 3.26)
project(mmo_mock_client)

file(GLOB FILES
    src/*.cpp
)

add_executable(${PROJECT_NAME} ${FILES})

target_link_libraries(${PROJECT_NAME} 
        PUBLIC 
        towards
        mmo::network
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
