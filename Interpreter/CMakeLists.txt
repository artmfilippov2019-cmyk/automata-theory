cmake_minimum_required(VERSION 3.22)
project(Interpreter)

set(CMAKE_CXX_STANDARD 20)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

BISON_TARGET(Parser src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.tab.h)
FLEX_TARGET(Lexer src/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.cpp)
ADD_FLEX_BISON_DEPENDENCY(Lexer Parser)

add_executable(Interpreter
    main.cpp
    src/value.cpp
    src/symbol_table.cpp
    src/maze.cpp
    src/robot.cpp
    src/interpreter.cpp
    src/visualizer.cpp
    ${BISON_Parser_OUTPUTS}
    ${FLEX_Lexer_OUTPUTS}
)

target_include_directories(Interpreter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

if(MSVC)
    target_compile_options(Interpreter PRIVATE /utf-8)
endif()

file(COPY ${CMAKE_SOURCE_DIR}/mazes DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/programs DESTINATION ${CMAKE_BINARY_DIR})
