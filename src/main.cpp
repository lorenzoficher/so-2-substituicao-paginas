// Entry point of build/sim; everything else lives in cli.cpp.
#include <iostream>
#include <string>
#include <vector>

#include "cli.hpp"

int main(int argc, char* argv[]) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    return run_cli(args, std::cout, std::cerr);
}
