#include <iostream>
#include <vector>
#include <string>
#include "src/hider_seeker/seeker/seeker_model_init.hpp"

int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    return hider_seeker::runSeekerModelInit(args, std::cout, std::cerr);
}