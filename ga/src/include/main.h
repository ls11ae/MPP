#pragma once

#include <iostream>

#include "problem.hpp"
#include "miscellaneous.h"
#include "geneticalgorithm.h"
#include "incrementalnofitsolver.h"
#include "innerfitraster.h"
#include "h4np.h"
#include "clusteredproblem.h"
#include "beamsearch.h"

bool visualize = false;
bool storeSolution;
std::string outputLocation;
Metaheuristic meta = Metaheuristic::NONE;
AlgorithmType algorithm = AlgorithmType::BEST_FIT;
json metaSpecifics;
json algorithmSpecifics;
double clustering = 0.0;
double deleteItems = 0.0;

GASpecifics GAHyperparams({
    10, //populationSize;
    10, //generations;
    {}, //leave blank
    0.2, //mutationRate;
    0.6, //crossoverRate;
    "SingleSwap", //mutationType;
    "OX", //crossoverType;
    true //optimize strategy
});