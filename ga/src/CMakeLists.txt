cmake_minimum_required(VERSION 3.27)
set (CMAKE_CXX_STANDARD 23)
project(MPP)
file (GLOB_RECURSE sources *.h *.cpp *.hpp *.tpp)

find_package(CGAL REQUIRED)
#find_package(PkgConfig REQUIRED)

include_directories(include tools)

#pkg_check_modules(CLIPPER REQUIRED IMPORTED_TARGET Clipper2Z)

add_executable(mpp ${sources})
#target_link_libraries(mpp CGAL::CGAL PkgConfig::CLIPPER)
add_compile_definitions(USINGZ)