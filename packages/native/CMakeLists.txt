cmake_minimum_required(VERSION 3.18)

project(vsrender)

add_library(${PROJECT_NAME} SHARED)

target_sources(${PROJECT_NAME} PRIVATE
    src/vsrender.c
    src/utils.c
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    SUFFIX ".node"
)

target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_JS_SRC})
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC})

find_package(PkgConfig REQUIRED)

pkg_search_module(VSS REQUIRED IMPORTED_TARGET vapoursynth-script)
target_link_libraries(${PROJECT_NAME} PkgConfig::VSS)

target_compile_definitions(${PROJECT_NAME} PRIVATE LIB_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}")
