cmake_minimum_required(VERSION 3.22)
project(ta2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.6.0
)
FetchContent_MakeAvailable(Catch2)

add_library(ta2_regex STATIC
        ast/ast.cpp
        ast_builder/ast_builder.cpp
        nfa/nfa.cpp
        dfa/dfa.cpp
)
target_include_directories(ta2_regex PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(ta2 main.cpp
        my_regex/my_regex.cpp
        my_regex/my_regex.h)
target_link_libraries(ta2 PRIVATE ta2_regex)

add_executable(ta2_tests
        tests/regex_tests.cpp
        my_regex/my_regex.cpp
        my_regex/my_regex.h)
target_link_libraries(ta2_tests PRIVATE ta2_regex Catch2::Catch2WithMain)

add_test(NAME ta2_tests COMMAND ta2_tests)
