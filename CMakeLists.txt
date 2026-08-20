cmake_minimum_required(VERSION 3.30)

project(SkinSelector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)

FetchContent_Declare(
  MinHook
  GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
  GIT_TAG        master
)

FetchContent_MakeAvailable(MinHook)

add_library(SkinSelector SHARED)

target_compile_definitions(SkinSelector
    PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

target_sources(SkinSelector
    PRIVATE
        src/dllmain.cpp
        src/Core.cpp
        src/res/loadgroup.cpp
)

target_include_directories(SkinSelector
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(SkinSelector  PRIVATE include minhook/include)
target_include_directories(SkinSelector  PRIVATE include thirdparty/mINI/src/mini)
target_link_libraries(SkinSelector  PRIVATE minhook)

set_target_properties(SkinSelector  PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        SUFFIX ".asi"
)