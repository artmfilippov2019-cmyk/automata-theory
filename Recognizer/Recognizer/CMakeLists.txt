find_package(FLEX REQUIRED)
find_program(JAVA_EXECUTABLE java REQUIRED)

if (NOT SMC_JAR)
    file(GLOB _smc_jar_candidates
            "$ENV{HOME}/smc/*/bin/Smc.jar"
            "/usr/local/share/smc/*/bin/Smc.jar"
            "/usr/share/smc/*/bin/Smc.jar")
    if (_smc_jar_candidates)
        list(GET _smc_jar_candidates 0 SMC_JAR)
    endif ()
    unset(_smc_jar_candidates)
endif ()
if (NOT SMC_JAR)
    find_file(SMC_JAR
            NAMES Smc.jar smc.jar
            HINTS
            ENV SMC_HOME
            "$ENV{HOME}/smc"
            /usr/local/share/smc
            /usr/share/smc
            /usr/share/java
            PATH_SUFFIXES bin lib
            DOC "SMC compiler jar")
endif ()
if (NOT SMC_JAR)
    message(FATAL_ERROR
            "SMC jar (Smc.jar) not found. ")
endif ()

find_path(STATEMAP_INCLUDE_DIR statemap.h
        HINTS
        ENV SMC_HOME
        /usr/local/include
        /usr/include
        PATH_SUFFIXES include C++
        DOC "Directory containing statemap.h")
if (NOT STATEMAP_INCLUDE_DIR)
    message(FATAL_ERROR
            "statemap.h not found. ")
endif ()

set(FLEX_GENERATED_CPP "${CMAKE_CURRENT_BINARY_DIR}/FlexRecognizer.cpp")
set(SMC_GENERATED_CPP "${CMAKE_CURRENT_BINARY_DIR}/SmcRecognizer_sm.cpp")
set(SMC_GENERATED_H "${CMAKE_CURRENT_BINARY_DIR}/SmcRecognizer_sm.h")

FLEX_TARGET(FlexRecognizer
        "${CMAKE_CURRENT_SOURCE_DIR}/FlexRecognizer.l"
        "${FLEX_GENERATED_CPP}")

add_custom_command(
        OUTPUT "${SMC_GENERATED_CPP}" "${SMC_GENERATED_H}"
        COMMAND "${JAVA_EXECUTABLE}" -jar "${SMC_JAR}"
        -c++
        -d .
        "${CMAKE_CURRENT_SOURCE_DIR}/SmcRecognizer.sm"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/SmcRecognizer.sm"
        "${SMC_JAR}"
        COMMENT "Generating SMC state machine from SmcRecognizer.sm")

add_library(recognizers
        "${FLEX_GENERATED_CPP}"
        "${SMC_GENERATED_CPP}"
        SmcRecognizer.cpp
        SmcRecognizer.hpp
        FlexRecognizer.hpp
        RegexRecognizer.cpp
        RegexRecognizer.hpp
        IRecognizer.hpp)

target_include_directories(recognizers PUBLIC
        "${CMAKE_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${STATEMAP_INCLUDE_DIR}")
target_link_libraries(recognizers PUBLIC Boost::regex)
