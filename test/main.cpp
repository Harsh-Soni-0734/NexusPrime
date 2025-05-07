#include <iostream>
#include <chrono>
#include "SQLParser.h"

// ------------------- Main Function -------------------
int main()
{
    Database db;
    SQLParser parser;
    std::cout << "\033[34mNexusPrime > \033[0m";

    // std::cout << "NexusPrime > ";
    std::string input;
    while (std::getline(std::cin, input))
    {
        if (input == "EXIT")
            break;
        auto query_start = std::chrono::high_resolution_clock::now();
        try
        {
            parser.parse(input, db);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << "\n";
        }
        auto query_end = std::chrono::high_resolution_clock::now();
        auto query_delta = query_end - query_start;
        auto µs = std::chrono::duration_cast<std::chrono::microseconds>(query_delta).count();
        std::cout << "\033[38;5;208mTotal time: " << µs << " µs\033[0m\n";

        std::cout << "\033[34mNexusPrime > \033[0m";
    }
    return 0;
}
