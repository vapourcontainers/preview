cmake_minimum_required(VERSION 3.20)

project(vscloud_native)

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

find_package(PkgConfig)

pkg_search_module(VPS REQUIRED IMPORTED_TARGET vapoursynth-script)
target_link_libraries(${PROJECT_NAME} PkgConfig::VPS)
