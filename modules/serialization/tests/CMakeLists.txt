project(tw_serialization_tests)

set(LIBS
    tw::network
)

file(GLOB FILES
    ./*.cpp
)

# add_library(${PROJECT_NAME}_sources OBJECT ${FILES})
# target_link_libraries(${PROJECT_NAME}_sources
#     Catch2::Catch2WithMain
#     ${LIBS}
#     tl::expected
#     tw::protocol
# )

# add_executable(${PROJECT_NAME})
add_executable(SerializationBenchmarks ./SerializationBenchmarks.cpp)

target_compile_definitions(SerializationBenchmarks PRIVATE TRACY_ON_DEMAND=1)

target_link_libraries(SerializationBenchmarks
    PRIVATE
    ${LIBS}
    # ${PROJECT_NAME}_sources
    towards
    tw::network
    tw::protocol
    tw::serialization
    Tracy::TracyClient
    TracyClient
    Catch2::Catch2WithMain
    tl::expected
    EnTT::EnTT
)

# target_link_libraries(${PROJECT_NAME}
#     PRIVATE
#     ${LIBS}
#     ${PROJECT_NAME}_sources
#     Catch2::Catch2WithMain
#     tl::expected
#     EnTT::EnTT
# )

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

include(CTest)
include(Catch)

# catch_discover_tests(${PROJECT_NAME})
