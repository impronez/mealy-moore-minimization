#pragma once
#ifndef MEALY_AUTOMATA_H
#define MEALY_AUTOMATA_H

#include <algorithm>
#include <fstream>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>

#include "IAutomata.h"

using inputSymbol = std::string;
using MealyTransitionTable = std::list<std::pair<inputSymbol, std::vector<Transition>>>;
using MealyStates = std::vector<std::string>;

class MealyAutomata final : public IAutomata
{
public:
    static constexpr char STATE_CHAR = 'X';
    static constexpr size_t FIRST_STATE_INDEX = 1;

    MealyAutomata(MealyStates states, MealyTransitionTable table)
        : m_states(std::move(states)),
        m_transitionTable(std::move(table))
    {}

    void ExportToCsv(const std::string &filename) const override
    {
        std::ofstream output(filename);
        if (!output.is_open())
        {
            const std::string message = "Could not open file " + filename + " for writing";
            throw std::invalid_argument(message);
        }

        for (const auto& state: m_states)
        {
            output << ';' << state;
        }
        output << std::endl;

        for (const auto& [inputSymbol, transitions] : m_transitionTable)
        {
            output << inputSymbol;

            for (const Transition& transition : transitions)
            {
                output << ';' << transition.nextState << '/' << transition.output;
            }

            output << std::endl;
        }
    }

    void Minimize() override
    {
        RemoveImpossibleState();

        size_t index = FIRST_STATE_INDEX;

        MealyStates newStates;

        std::map<std::string, std::string> minimizatedStates; //current -> new
        std::map<std::vector<std::string>, std::string> statesTransitions;
        std::map<std::string, std::vector<Transition>> newStateToTransitionsColumn;

        for (size_t j = 0; j < m_states.size(); j++)
        {
            std::vector<std::string> transitionsColumn;
            std::vector<Transition> transitions;

            for (auto& row: m_transitionTable)
            {
                transitionsColumn.emplace_back(row.second.at(j).output);
                transitions.push_back(row.second.at(j));
            }

            if (!statesTransitions.contains(transitionsColumn))
            {
                std::string newState = STATE_CHAR + std::to_string(index++);

                statesTransitions[transitionsColumn] = newState;

                minimizatedStates[m_states.at(j)] = newState;

                newStateToTransitionsColumn[newState] = transitions;

                newStates.emplace_back(newState);
            }
            else
            {
                std::string oldState = m_states.at(j);
                std::string newState = statesTransitions[transitionsColumn];

                minimizatedStates[oldState] = newState;
            }
        }

        m_states = newStates;

        m_transitionTable = GetNewTransitionTable(minimizatedStates, newStateToTransitionsColumn);
    }

private:

    MealyTransitionTable GetNewTransitionTable(std::map<std::string, std::string>& oldToNewStates,
        std::map<std::string, std::vector<Transition>>& newStateToTransitionsColumn) const
    {
        MealyTransitionTable table;
        auto inputSymbols = GetInputSymbols();

        std::map<std::string, std::vector<Transition>> newTransitionsTable;

        for (auto& it: newStateToTransitionsColumn)
        {
            size_t rowIndex = 0;

            for (auto transition: it.second)
            {
                transition.nextState = oldToNewStates.at(transition.nextState);

                newTransitionsTable[inputSymbols[rowIndex]].push_back(transition);

                rowIndex++;
            }
        }

        for (auto& it: newTransitionsTable)
        {
            table.emplace_back(it);
        }

        return table;
    }

    void RemoveImpossibleState()
    {
        auto possibleStatesSet = GetAllPossibleStatesSet(m_transitionTable, m_states);
        if (possibleStatesSet.size() == m_states.size())
        {
            return;
        }

        MealyStates newStates;
        std::vector<int> indexesToRemove;

        for (size_t i = 0; auto& it: m_states)
        {
            if (possibleStatesSet.contains(it))
            {
                newStates.emplace_back(it);
            }
            else
            {
                indexesToRemove.insert(indexesToRemove.begin(), i);
            }

            i++;
        }

        m_states = newStates;

        for (auto& row: m_transitionTable)
        {
            for (auto i: indexesToRemove)
            {
                row.second.erase(row.second.begin() + i);
            }
        }
    }

    static std::set<std::string> GetAllPossibleStatesSet(MealyTransitionTable& transitionTable, MealyStates& mealyStates)
    {
        std::vector<std::string> possibleStates = { mealyStates.front() };
        std::set<std::string> possibleStatesSet = { mealyStates.front() };
        size_t possibleStatesIndex = 0;

        while (possibleStatesIndex < possibleStates.size())
        {
            std::string sourceState = possibleStates[possibleStatesIndex];
            size_t index = GetIndexOfStringInVector(mealyStates, possibleStates.at(possibleStatesIndex++));

            for (auto& it: transitionTable)
            {
                std::string state = it.second[index].nextState;
                if (!possibleStatesSet.contains(state))
                {
                    possibleStatesSet.insert(state);
                    possibleStates.push_back(state);
                }
            }
        }

        return possibleStatesSet;
    }

    static size_t GetIndexOfStringInVector(std::vector<std::string>& states, std::string& state)
    {
        auto it = std::find(states.begin(), states.end(), state);

        if (it != states.end())
        {
            return std::distance(states.begin(), it);
        }

        throw std::range_error("Invalid state");
    }

    [[nodiscard]] std::vector<std::string> GetInputSymbols() const
    {
        std::vector<std::string> inputSymbols;

        for (auto& it: m_transitionTable)
        {
            inputSymbols.emplace_back(it.first);
        }

        return inputSymbols;
    }

    MealyStates m_states;
    MealyTransitionTable m_transitionTable;

};

#endif