project(tw_protocol)

set(GLOB FILES
    ./src/messages/*.cpp
    ./src/*.cc
)

set(GLOB HEADERS
    ./src/messages/*.hpp
    ./src/*.pb.h
    ./src/messages/*.h
)

file(GLOB PROTO_FILES
    ${CMAKE_CURRENT_LIST_DIR}/proto/*.proto
)

set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

add_library(${PROJECT_NAME} OBJECT ./src/Protocol.cpp ${PROTO_FILES})
add_library(tw::protocol ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    glm::glm
    protobuf::libprotobuf
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
    "$<BUILD_INTERFACE:${PROTO_BINARY_DIR}>"
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    ./src/
)

include(FindProtobuf)
find_package(Protobuf CONFIG REQUIRED)
protobuf_generate(
    TARGET ${PROJECT_NAME}
    IMPORT_DIRS "${CMAKE_CURRENT_LIST_DIR}/proto"
    PROTOC_OUT_DIR "${PROTO_BINARY_DIR}")


#set_target_properties(${PROJECT_NAME} PROPERTIES
#    POSITION_INDEPENDENT_CODE 1
#)

#set_source_files_properties(PROTO_SRC PROTO_HDRS PROPERTIES GENERATED TRUE)

target_sources(${PROJECT_NAME}
    PUBLIC FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_BINARY_DIR}
    FILES ${HEADERS} ${PROTO_HDRS})
