cmake_minimum_required (VERSION 3.15)

project (libprom VERSION 0.0.1 LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_subdirectory(prom)
add_subdirectory(promhttp)
add_subdirectory(example)
