set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB SHADERS ${SHADER_DIR}/*.vert
        ${SHADER_DIR}/*.frag
        ${SHADER_DIR}/*.comp)

foreach(SHADER IN LISTS SHADERS)
    get_filename_component(FILENAME ${SHADER} NAME)
    set(OUTPUT_FILE ${FILENAME}.spirv) 
    add_custom_command(OUTPUT ${OUTPUT_FILE} 
        COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -gVS ${SHADER} -V -o ${OUTPUT_FILE}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader: ${SHADER}"
    )

    list(APPEND SPV_SHADERS ${OUTPUT_FILE})
endForeach()

add_custom_target(tw_shaders ALL DEPENDS ${SPV_SHADERS})
