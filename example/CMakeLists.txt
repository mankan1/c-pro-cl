project(prom_example LANGUAGES C)

include(FindThreads)
find_library(microhttpd REQUIRED)

add_executable (prom_example bar.h bar.c foo.h foo.c main.c)

target_link_libraries (prom_example PRIVATE Threads::Threads prom microhttpd promhttp)

install (TARGETS prom_example DESTINATION bin)
