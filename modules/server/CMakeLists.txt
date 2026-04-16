cmake_minimum_required(VERSION 3.26)
project(tw_server)

# set(CMAKE_CXX_CLANG_TIDY "/usr/bin/clang-tidy;-checks=*")

file(GLOB FILES
    src/WorldServer.cpp
    src/interest_management/*.cpp
    src/replication/*.cpp
    src/systems/*.cpp
    src/monitoring/*.cpp
    src/network/*.cpp
)

add_library(tw_server_lib STATIC ${FILES})

target_include_directories(tw_server_lib
        PUBLIC
        ${PROJECT_SOURCE_DIR}/src/
)

target_link_libraries(tw_server_lib
        PUBLIC
        towards
        mmo::network
        tw::protocol
        tw::serialization

        PUBLIC
        glm::glm
        EnTT::EnTT
        Jolt
        protobuf::libprotobuf
        ${Boost_LIBRARIES}
        Tracy::TracyClient
        TracyClient
        pqxx
        pq
)

add_executable(${PROJECT_NAME} src/server.cpp)

target_link_libraries(tw_server
        PUBLIC
        tw_server_lib
)

ADD_SUBDIRECTORY(./tests/)
