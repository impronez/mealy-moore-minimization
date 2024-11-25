#pragma once
#include <stdexcept>
#include <string>

const std::string MEALY = "mealy";
const std::string MOORE = "moore";

enum class Automata
{
    Mealy,
    Moore
};

struct Args
{
    Automata automata;
    std::string inputFilename;
    std::string outputFilename;
};

inline Args ParseArgs(const int argc, char** argv)
{
    if (argc != 4)
    {
        throw std::invalid_argument("Invalid number of arguments. Must be: <automata> <inputFilename> <outputFilename>");
    }

    Automata automata;

    if (argv[1] == MEALY)
    {
        automata = Automata::Mealy;
    }
    else if (argv[1] == MOORE)
    {
        automata = Automata::Moore;
    }
    else
    {
        throw std::invalid_argument("Invalid automata");
    }

    return { automata, argv[2], argv[3] };
}
