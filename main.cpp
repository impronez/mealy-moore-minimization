#include <iostream>

#include "ArgumentsParser.h"
#include "AutomataController.h"
#include "Automata/IAutomata.h"

void MealyMinimization(Args& args)
{
    auto automata = MealyController::GetMealyAutomataFromCsvFile(args.inputFilename);

    automata->Minimize();

    automata->ExportToCsv(args.outputFilename);
}

void MooreMinimization(Args& args)
{
    auto automata = MooreController::GetMooreAutomataFromCsvFile(args.inputFilename);

    automata->Minimize();

    automata->ExportToCsv(args.outputFilename);
}

int main(const int argc, char** argv)
{
    try
    {
        switch (Args args = ParseArgs(argc, argv); args.automata)
        {
            case Automata::Mealy:
                MealyMinimization(args);
                break;
            case Automata::Moore:
                MooreMinimization(args);
                break;
            default: break;
        }

        std::cout << "Executed!\n";
    }
    catch (const std::exception& err)
    {
        std::cout << err.what() << std::endl;
        return -1;
    }
    return 0;
}
