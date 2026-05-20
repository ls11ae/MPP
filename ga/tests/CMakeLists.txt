
include_directories(../src/include)
include_directories(../src/include/solvers)
include_directories(../src/tools)

file(GLOB_RECURSE lib_SOURCES "../src/*.cpp" "../src/*/*.cpp")
file(GLOB_RECURSE test_SOURCES "../tests/*.cpp")

find_package(CGAL)
find_package(Catch2 REQUIRED)
# These tests can use the Catch2-provided main
add_executable(tests ${test_SOURCES} ../src/problem.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain CGAL::CGAL)

#include(CTest)
#include(Catch)
#catch_discover_tests(tests)