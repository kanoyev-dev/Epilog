cmake_minimum_required(VERSION 3.14)
project(sscc_validator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)
endif()

add_library(sscc_validator sscc_validator.cpp)
target_include_directories(sscc_validator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(main main.cpp)
target_link_libraries(main PRIVATE sscc_validator)

enable_testing()
add_executable(test_sscc test_sscc.cpp)
target_link_libraries(test_sscc PRIVATE sscc_validator)
add_test(NAME test_sscc COMMAND test_sscc)
