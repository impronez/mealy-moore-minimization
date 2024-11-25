#pragma once
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#include "ArgumentsParser.h"
#include "Automata/MealyAutomata.h"
#include "Automata/MooreAutomata.h"

namespace MealyController
{
    inline std::vector<std::string> GetStatesFromFile(std::ifstream& inputFile)
    {
        std::vector<std::string> states;

        std::string line;
        std::getline(inputFile, line);

        std::stringstream ss(line);
        std::string state;

        std::getline(ss, state, ';');

        while (std::getline(ss, state, ';'))
        {
            states.push_back(state);
        }

        return states;
    }

    inline MealyTransitionTable GetTransitionsFromFile(std::ifstream& inputFile, std::vector<std::string>& states)
    {
        MealyTransitionTable transitionTable;

        std::string line;
        while (std::getline(inputFile, line))
        {
            std::stringstream ss(line);
            std::string inputSymbol;
            std::getline(ss, inputSymbol, ';');

            std::vector<Transition> transitions;

            for (const auto& state : states)
            {
                std::string transitionData;
                if (std::getline(ss, transitionData, ';'))
                {
                    size_t separatorPos = transitionData.find('/');
                    if (separatorPos != std::string::npos)
                    {
                        std::string nextState = transitionData.substr(0, separatorPos);
                        std::string output = transitionData.substr(separatorPos + 1);

                        transitions.emplace_back(nextState, output);
                    }
                }
            }

            transitionTable.emplace_back(inputSymbol, transitions);
        }

        return transitionTable;
    }

    inline std::unique_ptr<MealyAutomata> GetMealyAutomataFromCsvFile(const std::string &inputFilename)
    {
        std::ifstream input(inputFilename);
        if (!input.is_open())
        {
            std::string message = "File \"" + inputFilename + "\" not found";
            throw std::runtime_error(message);
        }

        std::vector<std::string> states = GetStatesFromFile(input);
        MealyTransitionTable transitions = GetTransitionsFromFile(input, states);

        input.close();

        MealyAutomata mealy(std::move(states), std::move(transitions));
        return std::make_unique<MealyAutomata>(mealy);
    }
}

namespace MooreController
{
    inline std::vector<std::string> GetOutputSymbolsFromFile(std::istream& input)
    {
        std::vector<std::string> outputSymbols;

        if (std::string line; std::getline(input, line))
        {
            std::stringstream ss(line);
            std::string outputSymbol;
            bool isFirstReading = true;
            while (std::getline(ss, outputSymbol, ';'))
            {
                if (isFirstReading)
                {
                    isFirstReading = false;
                    continue;
                }

                outputSymbols.push_back(outputSymbol);
            }
        }

        return outputSymbols;
    }

    inline MooreStatesInfo GetStatesFromFile(std::istream& input, const std::vector<std::string>& outputSymbols)
    {
        MooreStatesInfo states;
        std::string line;

        if (std::getline(input, line))
        {
            size_t index = 0;
            std::stringstream ss(line);
            std::string state;
            while (std::getline(ss, state, ';'))
            {
                if (!state.empty())
                {
                    states.emplace_back(state, outputSymbols.at(index++));
                }
            }
        }

        return states;
    }

    inline std::unique_ptr<MooreAutomata> GetMooreAutomataFromCsvFile(const std::string& filename)
    {
        std::list<std::pair<std::string, std::vector<std::string>>> transitionTable;

        std::vector<std::string> inputSymbols;
        std::vector<std::string> outputSymbols;
        MooreStatesInfo states;

        std::ifstream file(filename);
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open the file.");
        }

        std::string line;

        outputSymbols = GetOutputSymbolsFromFile(file);

        states = GetStatesFromFile(file, outputSymbols);

        while (std::getline(file, line))
        {
            std::stringstream ss(line);
            std::string inputSymbol;
            if (std::getline(ss, inputSymbol, ';'))
            {
                std::vector<std::string> stateTransitions;
                std::string transition;
                while (std::getline(ss, transition, ';'))
                {
                    if (!transition.empty())
                    {
                        stateTransitions.push_back(transition);
                    }
                }
                inputSymbols.push_back(inputSymbol);
                transitionTable.emplace_back(inputSymbol, stateTransitions);
            }
        }
        MooreAutomata mooreAutomata(std::move(inputSymbols), std::move(states), std::move(transitionTable));
        return std::make_unique<MooreAutomata>(mooreAutomata);
    }
}