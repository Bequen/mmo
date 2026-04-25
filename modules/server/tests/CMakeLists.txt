project(tw_server_tests)

# set(CMAKE_CXX_CLANG_TIDY "/usr/bin/clang-tidy;-checks=*")

file(GLOB FILES
  ./*.cpp
)

add_executable(${PROJECT_NAME} ${FILES})

target_compile_options(${PROJECT_NAME} PRIVATE -UTRACY_CALLSTACK)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${PROJECT_SOURCE_DIR}/
        # ${quadtree_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        tw_server_lib
        Tracy::TracyClient
        TracyClient
)
