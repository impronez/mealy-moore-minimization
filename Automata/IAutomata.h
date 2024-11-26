#pragma once
#include <memory>
#include <string>

using State = std::string;
using InputSymbol = std::string;
using OutputSymbol = std::string;

struct Transition
{
    Transition(std::string& nextState, std::string& output)
        : nextState(nextState),
        output(output)
    {}

    std::string nextState;
    std::string output;

    bool operator<(const Transition& other) const
    {
        if (nextState != other.nextState)
        {
            return nextState < other.nextState;
        }
        return output < other.output;
    }
};

class Group
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

    [[nodiscard]] std::set<std::string> GetStates() const
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
    std::set<std::string> m_states {};
};

class IAutomata
{
public:
    virtual void ExportToCsv(const std::string& filename) const = 0;

    virtual void Minimize() = 0;

    virtual ~IAutomata() = default;
};