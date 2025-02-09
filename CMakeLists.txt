cmake_minimum_required(VERSION 3.28.2)
project(julmalloc LANGUAGES C VERSION 1.0.0 DESCRIPTION "julmalloc")

add_compile_options(-Wall -Wextra -Wno-unused-variable -std=gnu23)
include(CTest)

install(CODE "
  message(STATUS \"PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}\")
  message(STATUS \"CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}\")
  message(STATUS \"CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}\")
")

include_directories(".")

add_subdirectory(alloc)
add_subdirectory(unittests)
