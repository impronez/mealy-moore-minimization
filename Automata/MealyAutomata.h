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

class MealyGroup
{
public:
    void AddState(const std::string& state)
    {
        m_states.emplace(state);
    }

    void RemoveState(const std::string& state)
    {
        if (m_states.contains(state))
        {
            m_states.erase(state);
        }
    }

    std::string GetMainState()
    {
        for (auto& state : m_states)
        {
            return state;
        }

        throw std::runtime_error("No state found");
    }

    [[nodiscard]] std::set<std::string> GetStates() const
    {
        return m_states;
    }

    [[nodiscard]] unsigned GetStatesCount() const
    {
        return m_states.size();
    }

private:
    std::set<std::string> m_states;
};

using State = std::string;
using OutputSymbol = std::string;

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

        std::map<State, MealyGroup*> stateToGroup;
        std::map<std::vector<OutputSymbol>, std::vector<MealyGroup>> outputToGroup;

        std::map<State, unsigned> stateIndexes;
        for (unsigned i = 0; auto& state: m_states)
        {
            stateIndexes[state] = i++;
        }

        InitGroups(stateToGroup, outputToGroup);

        auto statesTransitions = GetStatesTransitions();

        while (true)
        {
            unsigned size = GetGroupsCount(outputToGroup);
            for (auto& pair: outputToGroup)
            {
                for (auto& group: pair.second)
                {
                    if (group.GetStatesCount() == 1)
                    {
                        continue;
                    }

                    std::string mainState = group.GetMainState();

                    std::vector<MealyGroup*> mainStateTransitions;
                    for (auto& it: statesTransitions[mainState])
                    {
                        mainStateTransitions.emplace_back(stateToGroup[it]);
                    }

                    for (auto& state: group.GetStates())
                    {
                        if (state == mainState)
                        {
                            continue;
                        }

                        for (unsigned i = 0; auto& it: statesTransitions[state])
                        {
                            if (mainStateTransitions.at(i++) != stateToGroup[it])
                            {
                                group.RemoveState(state);

                                MealyGroup newGroup;
                                newGroup.AddState(state);

                                pair.second.emplace_back(newGroup);
                                stateToGroup[state] = &pair.second.back();

                                break;
                            }
                        }

                    }
                }
            }

            if (GetGroupsCount(outputToGroup) == size)
            {
                break;
            }
        }

        BuildMinimizedAutomata(stateToGroup, outputToGroup);
    }

private:
    static constexpr char NEW_STATE_CHAR = 'X';
    void BuildMinimizedAutomata(std::map<State, MealyGroup*>& stateToGroup,
        std::map<std::vector<OutputSymbol>, std::vector<MealyGroup>> outputToGroup)
    {
        std::string inputState = m_states.front();
        MealyStates newStates;
        std::vector<std::pair<inputSymbol, std::vector<Transition>>> transitionTable;
        for (auto& row: m_transitionTable)
        {
            transitionTable.emplace_back(row.first, std::vector<Transition>());
        }

        auto newStateNames = GetNewStateNames(outputToGroup);

        auto statesTransitions = GetStatesTransitionsMap();

        for (auto& pair: outputToGroup)
        {
            for (auto& group: pair.second)
            {
                std::string state = group.GetMainState();
                std::string newState = newStateNames[state];

                auto transitions = statesTransitions[state];
                unsigned i = 0;
                if (state == inputState)
                {
                    newStates.emplace(newStates.begin(), newState);

                    for (auto& it: transitions)
                    {
                        std::string nextState = newStateNames[it.nextState];
                        transitionTable.at(i).second.emplace(transitionTable.at(i).second.begin(),
                                                    nextState, it.output);
                        ++i;
                    }
                }
                else
                {
                    newStates.emplace_back(newState);

                    for (auto& it: transitions)
                    {
                        std::string nextState = newStateNames[it.nextState];
                        transitionTable.at(i++).second.emplace_back(nextState, it.output);
                    }
                }
            }
        }

        MealyTransitionTable newTransitionTable;
        for (auto& it: transitionTable)
        {
            newTransitionTable.emplace_back(it.first, it.second);
        }

        m_states = newStates;
        m_transitionTable = newTransitionTable;
    }

    std::map<State, std::vector<Transition>> GetStatesTransitionsMap()
    {
        std::map<State, std::vector<Transition>> statesTransitions;
        for (unsigned i = 0; auto& state: m_states)
        {
            std::vector<Transition> transitions;
            for (auto& row: m_transitionTable)
            {
                transitions.emplace_back(row.second.at(i));
            }
            statesTransitions[state] = transitions;
            ++i;
        }

        return statesTransitions;
    }

    std::map<State, State> GetNewStateNames(
        std::map<std::vector<OutputSymbol>, std::vector<MealyGroup>>& outputToGroup) const
    {
        std::string inputState = m_states.front();
        std::map<State, State> newStates;
        for (int index = 1; auto& pair: outputToGroup)
        {
            for (auto& group: pair.second)
            {
                std::string oldStateName = group.GetMainState();
                std::string newStateName;
                if (oldStateName == inputState)
                {
                    newStateName = NEW_STATE_CHAR + std::to_string(0);
                }
                else
                {
                    newStateName = NEW_STATE_CHAR + std::to_string(index++);
                }

                for (auto& state: group.GetStates())
                {
                    newStates[state] = newStateName;
                }
            }
        }

        return newStates;
    }

    static unsigned GetGroupsCount(std::map<std::vector<OutputSymbol>, std::vector<MealyGroup>>& group)
    {
        unsigned size = 0;
        for (auto& it: group)
        {
            size += it.second.size();
        }

        return size;
    }

    std::map<State, std::vector<State>> GetStatesTransitions()
    {
        std::map<State, std::vector<State>> stateTransitions;
        for (unsigned i = 0; auto& state: m_states)
        {
            std::vector<State> transitions;
            for (auto& row: m_transitionTable)
            {
                transitions.emplace_back(row.second.at(i).nextState);
            }
            stateTransitions[state] = transitions;
            ++i;
        }

        return stateTransitions;
    }

    void InitGroups(std::map<State, MealyGroup*>& stateToGroup,
        std::map<std::vector<OutputSymbol>, std::vector<MealyGroup>>& outputToGroup) const
    {
        unsigned size = m_states.size();
        for (size_t i = 0; i < size; i++)
        {
            std::string state = m_states.at(i);

            std::vector<OutputSymbol> transitions;
            for (auto& it: m_transitionTable)
            {
                transitions.push_back(it.second.at(i).output);
            }

            if (!outputToGroup.contains(transitions))
            {
                MealyGroup group;
                group.AddState(m_states.at(i));

                outputToGroup[transitions].emplace_back(group);
                stateToGroup[state] = &outputToGroup[transitions].back();
            }
            else
            {
                stateToGroup[state] = &outputToGroup[transitions].back();
                outputToGroup[transitions].back().AddState(state);
            }
        }
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

    MealyStates m_states;
    MealyTransitionTable m_transitionTable;

};

#endif