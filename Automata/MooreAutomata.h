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
        std::string firstOutput = m_statesInfo.front().second;

        std::map<OutputSymbol, std::vector<MooreGroup>> groups;
        std::map<OutputSymbol, std::vector<State>> statesByOutput = GetOutputToStatesMap();
        std::map<State, unsigned> stateIndexes = GetStateIndexes();
        std::map<State, MooreGroup*> stateToGroup;

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
            unsigned size = GetGroupsSize(groups);
            for (auto& elem: groups)
            {
                for (auto& group: elem.second)
                {
                    std::vector<MooreGroup*> transitionGroups;
                    std::set<State> states = group.GetStates();

                    for (auto& state: states)
                    {
                        if (transitionGroups.empty())
                        {
                            transitionGroups = GetStateTransitionGroups(stateToGroup, stateIndexes[state]);
                            continue;
                        }

                        if (transitionGroups != GetStateTransitionGroups(stateToGroup, stateIndexes[state]))
                        {

                            elem.second.emplace_back();
                            elem.second.back().AddState(state);

                            group.RemoveState(state);

                            stateToGroup[state] = &elem.second.back();
                        }
                    }
                }
            }
            if (size == GetGroupsSize(groups))
            {
                break;
            }
        }

        BuildMinimizedAutomata(groups, stateIndexes, firstOutput);
    }

private:
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

    std::vector<MooreGroup*> GetStateTransitionGroups(std::map<State, MooreGroup*>& stateToGroup, unsigned index) const
    {
        std::vector<MooreGroup*> group;
        for (auto& row: m_transitionTable)
        {
            std::string state = row.second[index];
            group.emplace_back(stateToGroup[state]);
        }

        return group;
    }

    static unsigned GetGroupsSize(const std::map<OutputSymbol, std::vector<MooreGroup>>& groups)
    {
        unsigned size = 0;
        for (auto& it: groups)
        {
            size += it.second.size();
        }

        return size;
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