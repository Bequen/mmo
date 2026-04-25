project(tw_serialization)

# ── Library ──────────────────────────────────────────────────────────────────
# tw::serialization is a header-only INTERFACE library.
# All hot-path code is in inline functions / templates so the compiler can
# see everything in one translation unit and optimise accordingly.

add_library(tw_serialization INTERFACE)
add_library(tw::serialization ALIAS tw_serialization)

target_include_directories(tw_serialization
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/tw/serial/
)

target_link_libraries(tw_serialization
    INTERFACE
        glm::glm
        EnTT::EnTT
)

# Require C++20 for concepts / span
target_compile_features(tw_serialization INTERFACE cxx_std_20)

# ── Tests / benchmarks ───────────────────────────────────────────────────────
add_subdirectory(tests)
