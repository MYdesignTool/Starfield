cmake_minimum_required(VERSION 3.24)

project(StarfieldCore VERSION 0.1.0 LANGUAGES CXX)

add_library(starfield_core STATIC
    src/core/Settings.cpp
)
target_include_directories(starfield_core PUBLIC include)
target_compile_features(starfield_core PUBLIC cxx_std_20)
set_target_properties(starfield_core PROPERTIES
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON
)

if(MSVC)
    target_compile_options(starfield_core PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(starfield_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

