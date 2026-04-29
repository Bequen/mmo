project(tw_server_tests)

# set(CMAKE_CXX_CLANG_TIDY "/usr/bin/clang-tidy;-checks=*")

set(TEST_LIBS
    PUBLIC
    tw_server_lib
    Tracy::TracyClient
    TracyClient
)

function(add_server_test target source)
    add_executable(${target} ${source})
    target_compile_options(${target} PRIVATE -UTRACY_CALLSTACK)
    target_include_directories(${target} PUBLIC ${PROJECT_SOURCE_DIR}/)
    target_link_libraries(${target} ${TEST_LIBS})
endfunction()

add_server_test(tw_range_query_benchmarks RangeQueryBenchmarks.cpp)
add_server_test(tw_zone_manager_tests     ZoneManagerTests.cpp)
