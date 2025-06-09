cmake_minimum_required(VERSION 3.27)

project(WikiLoader)

# Allows clangd to find the library locations
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(fmt CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(re2 REQUIRED)

# Then use pkg-config for locate specific package
pkg_check_modules(libxmlpp REQUIRED IMPORTED_TARGET libxml++-5.0)

file(GLOB Src_Sources "./src/*.cpp")

add_executable(${CMAKE_PROJECT_NAME} ${Src_Sources})

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE fmt::fmt)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE PkgConfig::libxmlpp)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE re2::re2)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -DNDEBUG")
set(CMAKE_BUILD_TYPE Release)
