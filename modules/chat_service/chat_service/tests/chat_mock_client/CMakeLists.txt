cmake_minimum_required(VERSION 3.26)
project(tw_chat_mock_client)

add_executable(${PROJECT_NAME} ChatMockClient.cpp)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        tw::protocol
        tw::messaging
        mmo::network
        spdlog::spdlog
)
