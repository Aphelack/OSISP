cmake_minimum_required(VERSION 3.10)

# Define the executable
add_executable(async_io async_io.c)

# Set compile definitions for Unicode
target_compile_definitions(async_io PRIVATE
    UNICODE
    _UNICODE
)

# Set compiler flags
target_compile_options(async_io PRIVATE
    -Wall
    -O2
)

# Link required Windows libraries
target_link_libraries(async_io PRIVATE
    kernel32
    user32
)

# Set linker flags for console application with Unicode entry point
target_link_options(async_io PRIVATE
    -municode
)
