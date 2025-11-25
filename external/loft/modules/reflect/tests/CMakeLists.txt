project(loft_reflect_tests)

find_package(Vulkan)
find_package(Catch2 REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

include(CTest)
include(Catch)

add_executable(loft_reflect_tests 
    ./PipelineLayoutReflectTests.cpp
)

target_link_libraries(loft_reflect_tests 
    PRIVATE 
    loft::base
    loft::reflect
    Catch2::Catch2WithMain
)

catch_discover_tests(loft_reflect_tests)

