project(tw_chat_client)

file(GLOB FILES
    src/*.cpp
)

file(GLOB_RECURSE HEADERS
    include/*.hpp
)

add_library(${PROJECT_NAME} OBJECT ${FILES} ${PROTO_FILES})
add_library(tw::chat_client ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PUBLIC FILE_SET HEADERS
    BASE_DIRS include
    FILES ${HEADERS}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./include/
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        tw::network
        tl::expected
        tw::messaging
        tw::protocol
        protobuf::libprotobuf
        tw::chat::lib
        spdlog::spdlog
)
