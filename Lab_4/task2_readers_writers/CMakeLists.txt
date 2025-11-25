cmake_minimum_required(VERSION 3.10)

# Define executable
add_executable(readers_writers main.cpp)

# Set compile definitions for Unicode
target_compile_definitions(readers_writers PRIVATE UNICODE _UNICODE)

# Set compiler flags
target_compile_options(readers_writers PRIVATE -Wall -O2)

# Link required Windows libraries
target_link_libraries(readers_writers PRIVATE kernel32 user32)

# Set linker flags for console application with Unicode
target_link_options(readers_writers PRIVATE -municode)
