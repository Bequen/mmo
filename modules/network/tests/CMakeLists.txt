project(tw_network_tests)

find_package(Catch2 REQUIRED)

set(LIBS
    mmo::network
)

file(GLOB FILES
    ./*.cpp
    ./quicr/QuicrBasicTests.cpp
)

add_library(${PROJECT_NAME}_sources OBJECT ${FILES})
target_link_libraries(${PROJECT_NAME}_sources
    Catch2::Catch2WithMain
    ${LIBS}
    tl::expected
)

add_executable(${PROJECT_NAME})
add_executable(QuicrOverloadTest ./quicr/QuicrOverloadTests.cpp)

target_link_libraries(QuicrOverloadTest
    PRIVATE
    ${LIBS}
    ${PROJECT_NAME}_sources
    Catch2::Catch2WithMain
    tl::expected
    EnTT::EnTT
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${LIBS}
    ${PROJECT_NAME}_sources
    Catch2::Catch2WithMain
    tl::expected
    EnTT::EnTT
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

include(CTest)
include(Catch)

catch_discover_tests(${PROJECT_NAME})
