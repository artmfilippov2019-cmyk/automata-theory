add_library(generator_lib
        Generator.cpp
        Generator.hpp)

target_include_directories(generator_lib PUBLIC ${CMAKE_SOURCE_DIR})