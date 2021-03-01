cmake_minimum_required (VERSION 3.15)

project (libprom VERSION 0.0.1 LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(DEFINED ENV{USE_RWLOCKS})
    message(STATUS "using rwlocks")
    set(USE_RWLOCKS 1)
else()
    message(STATUS "using spinlocks")
    set(USE_RWLOCKS 0)
endif()
add_compile_definitions(USE_RWLOCKS=${USE_RWLOCKS})

add_subdirectory(prom)
add_subdirectory(promhttp)
add_subdirectory(example)
add_subdirectory(test)
