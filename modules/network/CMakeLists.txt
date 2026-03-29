cmake_minimum_required(VERSION 3.26)
project(mmo_network)


add_subdirectory(tests)

file(GLOB FILES
    src/*.cpp
    src/messenger/*.cpp
    src/protocol/quicr/*.cpp
    src/frames/*.cpp
)

file(GLOB HEADERS
    include/*.hpp
    include/exception/*.hpp
    include/io/*.hpp
    include/messenger/*.hpp
    include/packets/*.hpp
    include/metrics/*.hpp
    include/protocol/quicr/*.hpp
)

add_library(${PROJECT_NAME} OBJECT ${FILES})
add_library(mmo::network ALIAS mmo_network)
target_sources(${PROJECT_NAME}
    PUBLIC FILE_SET HEADERS
    BASE_DIRS include
    FILES ${HEADERS})

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE 1)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    ./include/
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    spdlog::spdlog
    tw::protocol
    tl::expected
    Tracy::TracyClient
    TracyClient
    EnTT::EnTT
)
