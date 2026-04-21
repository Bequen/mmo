cmake_minimum_required(VERSION 3.26)
project(tw_chat_server_exe)

add_executable(${PROJECT_NAME}
    src/ChatServerController.cpp
    src/ChatServerRuntime.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        src/
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        tw::chat::service
        tw::messaging
        tw::protocol
        mmo::network
        spdlog::spdlog
)
