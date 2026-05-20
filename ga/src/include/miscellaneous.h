#pragma once

#include "cxxopts.hpp"
#include <fstream>
#include "problem.hpp"

enum class Metaheuristic {
    NONE,
    GENETIC_ALGORITHM,
    H4NP
};

enum class AlgorithmType {
    DIRECT_INNER_FIT,
    INNER_FIT_RASTER,
    BEST_FIT,
    BEAM_SEARCH
};

extern bool visualize;
extern bool storeSolution;
extern std::string solution;
extern std::string comment;
extern std::string outputLocation;
extern AlgorithmType algorithm;
extern Metaheuristic meta;
extern json algorithmSpecifics;
extern json metaSpecifics;
extern GASpecifics GAHyperparams;
extern double clustering;
extern double deleteItems;

enum class SolveStatus { Optimal, Feasible, Unsolved };

void parseOptions(int argc, char* argv[]);