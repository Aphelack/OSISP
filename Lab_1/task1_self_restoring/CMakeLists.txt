cmake_minimum_required(VERSION 3.10)

# Define the executable
add_executable(self_restoring WIN32 self_restoring.c)

# Set compile definitions for Unicode
target_compile_definitions(self_restoring PRIVATE
    UNICODE
    _UNICODE
)

# Set compiler flags
target_compile_options(self_restoring PRIVATE
    -Wall
    -O2
)

# Link required Windows libraries
target_link_libraries(self_restoring PRIVATE
    kernel32
    user32
    gdi32
)

# Set linker flags for GUI application with Unicode entry point
target_link_options(self_restoring PRIVATE
    -mwindows
    -municode
)
