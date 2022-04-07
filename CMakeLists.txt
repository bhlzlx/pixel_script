cmake_minimum_required(VERSION 3.21)

project(make_compiler)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmakeconf" )

include(compiler_conf)
include(building_output)

add_subdirectory(src)

add_subdirectory(test)