cmake_minimum_required(VERSION 3.15) # For CMP0092
project(jocc C)

set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
  set(COMPILE_OPTIONS /W4 /WX)
else()
  set(COMPILE_OPTIONS -Wpedantic -Wall -Wextra -Werror -Wfatal-errors)
endif()

add_executable(jocc jocc/jocc.c)
target_compile_options(jocc PRIVATE ${COMPILE_OPTIONS})
