cmake_minimum_required(VERSION 3.29)
project(mealy_moore_minimization)

set(CMAKE_CXX_STANDARD 20)
if(WIN32)
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

add_executable(mealy_moore_minimization main.cpp
        Automata/IAutomata.h
        Automata/MealyAutomata.h
        Automata/MooreAutomata.h
        ArgumentsParser.h
        AutomataController.h)
