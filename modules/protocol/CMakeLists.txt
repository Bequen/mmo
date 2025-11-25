project(tw_protocol)

set(GLOB FILES 
    ./src/messages/*.cpp
)

add_library(${PROJECT_NAME} INTERFACE )
add_library(tw::protocol ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    INTERFACE
    glm::glm
)

target_include_directories(${PROJECT_NAME} 
    INTERFACE
    ./src/
)
