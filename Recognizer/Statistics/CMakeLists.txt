add_library(statistics_lib
        Statistics.cpp
        Statistics.hpp)

target_include_directories(statistics_lib PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(statistics_lib PUBLIC input_lib)
