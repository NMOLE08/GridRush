cmake_minimum_required(VERSION 3.16)
project(gridrush_sudoku_solver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(nlohmann_json CONFIG QUIET)

if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

add_executable(sudoku_solver
    src/main.cpp
    src/bitwise_solver.cpp
)

target_include_directories(sudoku_solver PRIVATE include)
target_link_libraries(sudoku_solver PRIVATE nlohmann_json::nlohmann_json)

add_executable(benchmark_classic
    src/benchmark_classic.cpp
    src/bitwise_solver.cpp
)

target_include_directories(benchmark_classic PRIVATE include)
target_link_libraries(benchmark_classic PRIVATE nlohmann_json::nlohmann_json)
