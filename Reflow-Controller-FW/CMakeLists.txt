cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()
message("🛠️ Build type: " ${CMAKE_BUILD_TYPE})

set(CMAKE_PROJECT_NAME reflow-controller)
project(${CMAKE_PROJECT_NAME} C CXX ASM)

# setup stm32cubemx library
add_subdirectory(cmake/stm32cubemx)
target_compile_options(stm32cubemx INTERFACE -w)

# output target
message("📦 Adding executable ${CMAKE_PROJECT_NAME}")
add_executable(${CMAKE_PROJECT_NAME})
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
        -Wall
        -Werror
        -Wextra
        -pedantic
)

file(GLOB_RECURSE reflow_srcs "src/*.c")
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${reflow_srcs})

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE src)
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
)
