cmake_minimum_required(VERSION 3.12)
project(Compiler)

find_package(REDasm REQUIRED)

file(GLOB_RECURSE HEADERS *.h*)
file(GLOB_RECURSE SOURCES *.cpp)

redasm_plugin(${PROJECT_NAME} ${PLUGIN_TYPE} ${HEADERS} ${SOURCES})
