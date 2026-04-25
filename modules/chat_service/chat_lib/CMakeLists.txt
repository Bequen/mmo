project(tw_chat_lib)

file(GLOB_RECURSE HEADERS
    include/*.hpp
)

add_library(${PROJECT_NAME} OBJECT ${HEADERS} ${PROTO_FILES})
add_library(tw::chat::lib ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ./include/
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        spdlog::spdlog
        tw::network
)
