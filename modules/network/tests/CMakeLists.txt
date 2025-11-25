project(tw_network_serializer_tests)

find_package(Catch2 REQUIRED)

set(LIBS
    mmo::network
)

set(FILES 
    ./SerializerTests.cpp
)

add_library(${PROJECT_NAME}_sources OBJECT ${FILES})
target_link_libraries(${PROJECT_NAME}_sources
    Catch2::Catch2WithMain
    ${LIBS}
)

add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    ${LIBS} 
    ${PROJECT_NAME}_sources
    Catch2::Catch2WithMain
)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

include(CTest)
include(Catch)

catch_discover_tests(${PROJECT_NAME})

