#pragma once

#include "problem.hpp"
#include "miscellaneous.h"

#include <vector>

class Solver {
    
public:
    virtual ~Solver() {};

    virtual SolveStatus solve(Problem* poly) = 0;

    virtual SolveStatus solve(Problem* prob, std::vector<size_t>& order) { return solve(prob); };

    virtual void setPlacementRule(int rule) {};

    virtual int getMaxPlacementRule() { return 0; };
};