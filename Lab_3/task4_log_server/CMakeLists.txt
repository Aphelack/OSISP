cmake_minimum_required(VERSION 3.10)

# Define executables
add_executable(log_server log_server.c)
add_executable(log_client log_client.c)

# Set compile definitions for Unicode
target_compile_definitions(log_server PRIVATE UNICODE _UNICODE)
target_compile_definitions(log_client PRIVATE UNICODE _UNICODE)

# Set compiler flags
target_compile_options(log_server PRIVATE -Wall -O2)
target_compile_options(log_client PRIVATE -Wall -O2)

# Link required Windows libraries
target_link_libraries(log_server PRIVATE kernel32)
target_link_libraries(log_client PRIVATE kernel32)

# Set linker flags for console applications with Unicode
target_link_options(log_server PRIVATE -municode)
target_link_options(log_client PRIVATE -municode)
