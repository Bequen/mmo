cmake_minimum_required(VERSION 3.26)
project(tw_messaging)

add_library(${PROJECT_NAME} INTERFACE)
add_library(tw::messaging ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} INTERFACE ./include/)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
        mmo::network
        tw::protocol
        tl::expected
        protobuf::libprotobuf
)
