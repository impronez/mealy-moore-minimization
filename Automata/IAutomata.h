#pragma once
#include <memory>
#include <string>

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

class IAutomata
{
public:
    virtual void ExportToCsv(const std::string& filename) const = 0;

    virtual void Minimize() = 0;

    virtual ~IAutomata() = default;
};