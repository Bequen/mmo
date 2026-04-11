cmake_minimum_required(VERSION 3.26)
project(tw_serialization)

add_subdirectory(tests)

file(GLOB FILES
    src/*.cpp
)

file(GLOB HEADERS
    include/*.hpp
)

# add_library(${PROJECT_NAME} OBJECT ${FILES})
# add_library(tw::serialization ALIAS tw_serialization)
# target_sources(${PROJECT_NAME}
#     PUBLIC FILE_SET HEADERS
#     BASE_DIRS include
#     FILES ${HEADERS})

# set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE 1)

# target_include_directories(${PROJECT_NAME}
#     PUBLIC
#     ./include/
# )

# target_link_libraries(${PROJECT_NAME}
#     PRIVATE
#     spdlog::spdlog
#     tw::protocol
#     tl::expected
#     Tracy::TracyClient
#     TracyClient
# )
