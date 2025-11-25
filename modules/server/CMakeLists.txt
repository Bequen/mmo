cmake_minimum_required(VERSION 3.26)
project(mmo_server)

file(GLOB FILES
    src/*.cpp
)

add_executable(${PROJECT_NAME} ${FILES} )

target_link_libraries(${PROJECT_NAME} 
        PUBLIC 
        towards
        mmo::network
        tw::protocol

        PRIVATE
        glm::glm
        EnTT::EnTT
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${PROJECT_SOURCE_DIR}/src/
)
