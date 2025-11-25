cmake_minimum_required(VERSION 3.10)
project(tcp_chat C)

set(CMAKE_C_STANDARD 99)

# Server
add_executable(chat_server server.c)
target_link_libraries(chat_server ws2_32)

# Client
add_executable(chat_client client.c)
target_link_libraries(chat_client ws2_32)
