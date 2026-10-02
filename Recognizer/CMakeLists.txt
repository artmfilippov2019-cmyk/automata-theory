cmake_minimum_required(VERSION 3.22)
project(ta1)

set(CMAKE_CXX_STANDARD 20)

option(BUILD_GENERATOR "Build Generator executable" ON)
option(BUILD_RECOGNIZER "Build Recognizer executable" ON)
option(BUILD_TIMECOUNTER "Build TimeCounter executable" ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

if (BUILD_RECOGNIZER OR BUILD_TIMECOUNTER)
    find_package(Boost CONFIG REQUIRED COMPONENTS regex)
    add_subdirectory(Recognizer)
endif()

if (BUILD_GENERATOR OR BUILD_TIMECOUNTER)
    add_subdirectory(Generator)
endif()

if (BUILD_RECOGNIZER)
    add_subdirectory(Input)
    add_subdirectory(Statistics)
endif()

if (BUILD_TIMECOUNTER)
    add_subdirectory(TimeCounter)
endif()

if (BUILD_RECOGNIZER)
    add_executable(Recognizer
            main.cpp)

    target_link_libraries(Recognizer PRIVATE recognizers statistics_lib input_lib)
endif()

if (BUILD_TIMECOUNTER)
    add_executable(TimeCounter
            TimeCounter/main.cpp)

    target_link_libraries(TimeCounter PRIVATE recognizers generator_lib timecounter_lib)
endif()

if (BUILD_GENERATOR)
    add_executable(Generator
            Generator/main.cpp)

    target_link_libraries(Generator PRIVATE generator_lib)
endif()