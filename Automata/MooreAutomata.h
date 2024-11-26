#pragma once

#ifndef MOORE_AUTOMATA_H
#define MOORE_AUTOMATA_H

#include <fstream>
#include <list>
#include <vector>

#include "IAutomata.h"

using InputSymbol = std::string;
using OutputSymbol = std::string;
using State = std::string;
using MooreTransitionTable = std::list<std::pair<InputSymbol, std::vector<State>>>;
using MooreStatesInfo = std::vector<std::pair<State, OutputSymbol>>;

class MooreGroup
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

    [[nodiscard]] std::set<State> GetStates() const
    {
        return m_states;
    }

    [[nodiscard]] size_t GetStatesCount() const
    {
        return m_states.size();
    }

    std::string GetMainState()
    {
        for (auto& state : m_states)
        {
            return state;
        }

        throw std::runtime_error("No state found");
    }

private:
    std::set<State> m_states {};
};

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

        std::string firstOutput = m_statesInfo.front().second;

        std::map<OutputSymbol, std::vector<MooreGroup>> groups;
        std::map<OutputSymbol, std::vector<State>> statesByOutput = GetOutputToStatesMap();
        std::map<State, unsigned> stateIndexes = GetStateIndexes();
        std::map<State, MooreGroup*> stateToGroup;

        auto statesTransitions = GetStatesTransitions();

        // To InitGroups()
        for (auto& it: statesByOutput)
        {
            for (auto& state: it.second)
            {
                if (!groups.contains(it.first))
                {
                    groups.emplace(it.first, std::vector<MooreGroup>());
                    groups[it.first].emplace_back();
                }
                groups.at(it.first).begin()->AddState(state);
                stateToGroup[state] = &groups.at(it.first).front();
            }
        }

        while (true)
        {
            bool isChangedSize = false;
            for (auto& pair: groups)
            {
                std::vector<MooreGroup> newGroups;
                for (auto& group: pair.second)
                {
                    if (group.GetStatesCount() <= 1)
                    {
                        continue;
                    }

                    std::string mainState = group.GetMainState();

                    std::vector<MooreGroup*> groupTransitions;
                    for (auto& it: statesTransitions[mainState])
                    {
                        groupTransitions.emplace_back(stateToGroup[it]);
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

                                MooreGroup newGroup;
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

        // for (auto& it: groups)
        // {
        //     for (auto& group: it.second)
        //     {
        //         std::cout << "Group: ";
        //         for (auto& state: group.GetStates())
        //         {
        //             std::cout << state << " ";
        //         }
        //         std::cout << std::endl;
        //     }
        // }

        BuildMinimizedAutomata(groups, stateIndexes, firstOutput);
    }

private:
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

    static constexpr char NEW_STATE_CHAR = 'X';

    void BuildMinimizedAutomata(std::map<OutputSymbol, std::vector<MooreGroup>>& groups,
        std::map<State, unsigned>& stateIndexes, const std::string& firstOutput)
    {
        MooreStatesInfo statesInfo;
        std::vector<std::pair<InputSymbol, std::vector<State>>> transitionTable;
        for (auto& row: m_transitionTable)
        {
            transitionTable.emplace_back(row.first, std::vector<State>());
        }

        std::map<State, State> stateToNewState = GetStateToNewStateMap(groups, firstOutput);

        for (auto& it: groups)
        {
            for (auto& group: it.second)
            {
                std::string state = group.GetMainState();
                unsigned stateIndex = stateIndexes[state];

                if (it.first == firstOutput)
                {
                    statesInfo.emplace(statesInfo.begin(), stateToNewState[state], it.first);
                }
                else
                {
                    statesInfo.emplace_back(stateToNewState[state], it.first);
                }

                for (unsigned i = 0; auto& row: m_transitionTable)
                {
                    std::string oldState = row.second[stateIndex];
                    std::string newState = stateToNewState[oldState];
                    if (it.first == firstOutput)
                    {
                        transitionTable.at(i).second.emplace(transitionTable.at(i).second.begin(), newState);
                    }
                    else
                    {
                        transitionTable.at(i).second.emplace_back(newState);
                    }
                    ++i;
                }
            }
        }
        MooreTransitionTable table;
        for (auto& it: transitionTable)
        {
            table.emplace_back(it.first, it.second);
        }

        m_statesInfo.swap(statesInfo);
        m_transitionTable.swap(table);
    }

    static std::map<State, State> GetStateToNewStateMap(std::map<OutputSymbol,
        std::vector<MooreGroup>>& groups, const std::string& firstOutput)
    {
        std::map<State, State> stateToNewState;
        for (unsigned index = 1; auto& it: groups)
        {
            for (auto& group: it.second)
            {
                for(auto& state: group.GetStates())
                {
                    if (it.first == firstOutput)
                    {
                        stateToNewState[state] = NEW_STATE_CHAR + std::to_string(0);
                        continue;
                    }
                    stateToNewState[state] = NEW_STATE_CHAR + std::to_string(index);
                }
                if (it.first != firstOutput)
                {
                    ++index;
                }
            }
        }

        return stateToNewState;
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

    std::vector<std::string> m_inputSymbols;
    MooreStatesInfo m_statesInfo;
    MooreTransitionTable m_transitionTable;
};

#endif