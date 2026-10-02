add_library(input_lib
        Input.cpp
        Input.hpp)

target_include_directories(input_lib PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(input_lib PUBLIC recognizers)
