#pragma once

#ifndef MOORE_AUTOMATA_H
#define MOORE_AUTOMATA_H

#include <fstream>
#include <list>
#include <vector>

#include "IAutomata.h"

using MooreTransitionTable = std::list<std::pair<InputSymbol, std::vector<State>>>;
using MooreStatesInfo = std::vector<std::pair<State, OutputSymbol>>;

class MooreAutomata final : public IAutomata
{
public:
    MooreAutomata(
        std::vector<InputSymbol>&& inputSymbols,
        MooreStatesInfo&& statesInfo,
        MooreTransitionTable&& transitionTable
    )
        : m_inputSymbols(std::move(inputSymbols)),
        m_statesInfo(std::move(statesInfo)),
        m_transitionTable(std::move(transitionTable))
    {}

    void ExportToCsv(const std::string &filename) const override
    {
        std::ofstream file(filename);
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open the file for writing.");
        }

        std::string statesStr, outputSymbolsStr;
        for (const auto& info : m_statesInfo)
        {
            outputSymbolsStr += ';' + info.second;
            statesStr += ';' + info.first;
        }
        outputSymbolsStr += '\n';
        statesStr += '\n';

        file << outputSymbolsStr;
        file << statesStr;

        for (const auto& input : m_inputSymbols)
        {
            file << input;

            for (const auto& transitions: m_transitionTable)
            {
                if (transitions.first == input)
                {
                    for (const auto& transition : transitions.second)
                    {
                        file << ";" << transition;
                    }
                    file << "\n";
                }
            }
        }

        file.close();
    }

    void Minimize() override
    {
        RemoveImpossibleStates();

        std::map<OutputSymbol, std::vector<Group>> groups;
        std::map<State, Group*> stateToGroup;
        InitGroups(groups, stateToGroup);

        std::map<State, unsigned> stateIndexes = GetStateIndexes();
        auto statesTransitions = GetStatesTransitions();

        while (true)
        {
            bool isChangedSize = false;
            for (auto& pair: groups)
            {
                std::vector<Group> newGroups;
                for (auto& group : pair.second)
                {
                    if (group.GetStatesCount() <= 1)
                    {
                        continue;
                    }

                    std::string mainState = group.GetMainState();
                    std::vector<Group*> groupTransitions;
                    for (auto& state: statesTransitions[mainState])
                    {
                        groupTransitions.emplace_back(stateToGroup[state]);
                    }

                    for (auto& state: group.GetStates())
                    {
                        if (state == mainState)
                        {
                            continue;
                        }

                        for (unsigned i = 0; auto& it: statesTransitions[state])
                        {
                            if (groupTransitions.at(i++) != stateToGroup[it])
                            {
                                group.RemoveState(state);

                                Group newGroup;
                                newGroup.AddState(state);

                                newGroups.push_back(newGroup);
                                stateToGroup[state] = &newGroups.back();

                                break;
                            }
                        }
                    }
                }

                for (auto& it: newGroups)
                {
                    pair.second.push_back(it);
                    if (!isChangedSize)
                    {
                        isChangedSize = true;
                    }
                }
            }

            if (!isChangedSize)
            {
                break;
            }
        }

        BuildMinimizedAutomata(groups, stateIndexes);
    }

private:
    static constexpr char NEW_STATE_CHAR = 'X';

    void BuildMinimizedAutomata(std::map<OutputSymbol, std::vector<Group>>& groups,
        std::map<State, unsigned>& stateIndexes)
    {
        std::map<State, State> newStateNames = GetNewStateNames(groups);

        std::vector<std::pair<InputSymbol, std::vector<State>>> transitionTableVector;
        for (auto& row: m_transitionTable)
        {
            transitionTableVector.emplace_back(row.first, std::vector<State>());
        }

        MooreStatesInfo newStatesInfo;
        MooreTransitionTable newTransitionTable;

        for (auto& pair: groups)
        {
            for (auto& group: pair.second)
            {
                std::string oldState = group.GetMainState();
                std::string newState = newStateNames[oldState];
                unsigned stateIndex = stateIndexes[oldState];
                std::string output = m_statesInfo[stateIndex].second;

                if (stateIndex == 0)
                {
                    newStatesInfo.emplace(newStatesInfo.begin(), newState, output);
                }
                else
                {
                    newStatesInfo.emplace_back(newState, output);
                }

                for (unsigned i = 0; auto& row: m_transitionTable)
                {
                    std::string nextState = row.second[stateIndex];
                    std::string newNextStateName = newStateNames[nextState];

                    if (stateIndex == 0)
                    {
                        transitionTableVector.at(i).second.emplace(
                            transitionTableVector.at(i).second.begin(), newNextStateName);
                    }
                    else
                    {
                        transitionTableVector.at(i).second.emplace_back(newNextStateName);
                    }
                    ++i;
                }
            }
        }

        for (auto& row: transitionTableVector)
        {
            newTransitionTable.emplace_back(row);
        }

        m_statesInfo = newStatesInfo;
        m_transitionTable = newTransitionTable;
    }

    std::map<State, State> GetNewStateNames(std::map<OutputSymbol, std::vector<Group>>& groups) const
    {
        std::string inputState = m_statesInfo.front().first;

        std::map<State, State> newStateNames;
        unsigned stateIndex = 1;

        for (auto it: groups)
        {
            for (auto& group: it.second)
            {
                std::string newStateName = group.GetMainState() == inputState
                    ? NEW_STATE_CHAR + std::to_string(0)
                    : NEW_STATE_CHAR + std::to_string(stateIndex++);
                for (auto& state: group.GetStates())
                {
                    newStateNames[state] = newStateName;
                }
            }
        }
        return newStateNames;
    }

    std::map<State, std::vector<State>> GetStatesTransitions()
    {
        std::map<State, std::vector<State>> statesTransitions;
        for (unsigned i = 0; auto& stateInfo: m_statesInfo)
        {
            std::vector<State> transitions;
            for (auto& row: m_transitionTable)
            {
                transitions.emplace_back(row.second.at(i));
            }
            statesTransitions[stateInfo.first] = transitions;
            ++i;
        }

        return statesTransitions;
    }

    std::map<State, unsigned> GetStateIndexes()
    {
        std::map<State, unsigned> stateIndexes;
        for (unsigned index = 0; auto& state: m_statesInfo)
        {
            stateIndexes[state.first] = index++;
        }

        return stateIndexes;
    }

    void InitGroups(std::map<OutputSymbol, std::vector<Group>>& groups,
        std::map<State, Group*>& stateToGroup)
    {
        for (auto& it: GetOutputToStatesMap())
        {
            for (auto& state: it.second)
            {
                if (!groups.contains(it.first))
                {
                    groups.emplace(it.first, std::vector<Group>());
                    groups[it.first].emplace_back();
                }
                groups.at(it.first).begin()->AddState(state);
                stateToGroup[state] = &groups.at(it.first).front();
            }
        }
    }

    std::map<OutputSymbol, std::vector<State>> GetOutputToStatesMap()
    {
        std::map<OutputSymbol, std::vector<State>> statesByOutput;

        for(auto& stateInfo: m_statesInfo)
        {
            if (!statesByOutput.contains(stateInfo.second))
            {
                statesByOutput.emplace(stateInfo.second, std::vector<State>());
            }
            statesByOutput.find(stateInfo.second)->second.emplace_back(stateInfo.first);
        }

        return statesByOutput;
    }

    void RemoveImpossibleStates()
    {
        auto possibleStatesSet = GetPossibleStates();
        if (possibleStatesSet.size() == m_statesInfo.size())
        {
            return;
        }

        MooreStatesInfo newStatesInfo;
        std::vector<unsigned> indexesToRemove;

        for (unsigned i = 0; auto& it: m_statesInfo)
        {
            if (possibleStatesSet.contains(it.first))
            {
                newStatesInfo.emplace_back(it);
            }
            else
            {
                indexesToRemove.emplace(indexesToRemove.begin(), i);
            }
            ++i;
        }

        for (auto& row: m_transitionTable)
        {
            for (auto& it: indexesToRemove)
            {
                row.second.erase(row.second.begin() + it);
            }
        }

        m_statesInfo = newStatesInfo;
    }

    std::set<State> GetPossibleStates()
    {
        std::vector<State> possibleStatesVector = { m_statesInfo.front().first };
        std::set<State> possibleStates = { m_statesInfo.front().first };
        size_t possibleStatesIndex = 0;

        while(possibleStatesIndex < possibleStatesVector.size())
        {
            std::string sourceState = possibleStatesVector[possibleStatesIndex++];
            size_t index = GetIndexOfState(m_statesInfo, sourceState);

            for (auto& it: m_transitionTable)
            {
                std::string state = it.second[index];
                if (!possibleStates.contains(state))
                {
                    possibleStates.insert(state);
                    possibleStatesVector.push_back(state);
                }
            }
        }

        return possibleStates;
    }

    static size_t GetIndexOfState(MooreStatesInfo& statesInfo, std::string& state)
    {
        unsigned index = 0;
        for (auto& stateInfo: statesInfo)
        {
            if (stateInfo.first == state)
            {
                return index;
            }

            ++index;
        }

        throw std::range_error("Invalid state");
    }


    std::vector<std::string> m_inputSymbols;
    MooreStatesInfo m_statesInfo;
    MooreTransitionTable m_transitionTable;
};

#endif