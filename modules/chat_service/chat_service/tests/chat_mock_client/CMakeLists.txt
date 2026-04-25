project(tw_chat_mock_client)

add_executable(${PROJECT_NAME} ChatMockClient.cpp)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        tw::protocol
        tw::messaging
        tw::network
        spdlog::spdlog
)
