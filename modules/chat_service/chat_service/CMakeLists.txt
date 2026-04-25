project(tw_chat_service)

file(GLOB FILES
    src/*.cpp
)

file(GLOB_RECURSE HEADERS
    include/*.hpp
)

add_library(${PROJECT_NAME} OBJECT ${FILES})
add_library(tw::chat::service ALIAS ${PROJECT_NAME})

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
        tw::chat::lib
        tl::expected
        spdlog::spdlog
)

add_subdirectory(tests)
