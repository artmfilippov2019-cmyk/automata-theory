add_library(timecounter_lib
        TimeCounter.cpp
        TimeCounter.hpp)

target_include_directories(timecounter_lib PUBLIC ${CMAKE_SOURCE_DIR})
