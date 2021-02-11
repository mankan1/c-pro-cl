include(CTest)
include(FindThreads)

file(GLOB HEADERS "*.h")
file(GLOB SOURCES "*.c")

add_executable(
    prom_test
    ${HEADERS}
    ${SOURCES}
)
target_link_libraries(prom_test microhttpd prom promhttp Threads::Threads)

add_test(NAME prom_test COMMAND prom_test)
