project(loft_reflect)

FetchContent_Declare(
    SpirvReflect
    GIT_REPOSITORY  https://github.com/KhronosGroup/SPIRV-Reflect 
    GIT_TAG         e55086b044225f9b511ae44dbf9b079d3625943f 
    GIT_SHALLOW     1
)

set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_EXAMPLES   OFF CACHE BOOL "" FORCE)
set(SPIRV_REFLECT_TESTS      OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SpirvReflect)

file(GLOB FILES
    ./src/*.cpp
)

file(GLOB HEADERS
    ./include/*.hpp
)

add_library(loft_reflect OBJECT ${FILES})
add_library(loft::reflect ALIAS loft_reflect)
target_sources(loft_reflect
    PUBLIC FILE_SET HEADERS
    BASE_DIRS include
    FILES ${HEADERS})

set_target_properties(loft_reflect PROPERTIES POSITION_INDEPENDENT_CODE 1)

target_link_libraries(${PROJECT_NAME} 
        PUBLIC 
        loft::base
        GPUOpen::VulkanMemoryAllocator 
        volk::volk
        spirv-reflect-static 
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
        ${PROJECT_SOURCE_DIR}/include/
)

# add_subdirectory(tests)
