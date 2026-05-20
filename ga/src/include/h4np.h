#pragma once

#include "solver.h"
#include "problem.hpp"
#include <execution>

template <class T>
class H4NP : public Solver
{
	static_assert(std::is_base_of<Solver, T>::value, "T must be of Type Solver");
public:
	H4NP(size_t iterations=100): iterations(iterations) {  };
	SolveStatus solve(Problem* prob);

private:
	size_t iterations;
	size_t batchSize = 50;
};



#include <random>

template<class T>
SolveStatus H4NP<T>::solve(Problem* prob)
{
	std::cout << prob->getString() << "\n";
	Solver* solver = static_cast<Solver*>(new T());
	std::vector<size_t> baseOrder;
	auto items = prob->getItems();
	for (size_t i = 0; i < items.size(); i++)
	{
		for (size_t j = 0; j < items[i]->quantity; j++) {
			baseOrder.push_back(i);
		}
	}

	int64_t bestScore = 0;
	std::vector<Candidate> bestCandidates;
	std::string bestStrategy;
	std::vector<int64_t> scores = {};
	std::vector<std::string> placementRules = {};

	auto rng = std::default_random_engine{};
	std::uniform_int_distribution<> ruleDistr(0, solver->getMaxPlacementRule());
	delete solver;

	std::vector<Problem*> problemCopies(batchSize, nullptr);
	for (size_t i = 0; i < problemCopies.size(); i++)
	{
		problemCopies[i] = prob->getCopy();
	}
	for (size_t i = 0; i < iterations; i += batchSize)
	{
		//std::for_each(std::execution::par, std::begin(problemCopies), std::end(problemCopies), [&](Problem* problemCopy)
		#pragma omp parallel for
		for (int i = 0; i < problemCopies.size(); i++)
		{
			problemCopies[i]->resetCandidates();
			Solver* solver = static_cast<Solver*>(new T());
			solver->setPlacementRule(ruleDistr(rng));
			auto order = baseOrder;
			std::shuffle(order.begin(), order.end(), rng);
			solver->solve(problemCopies[i], order);
			delete solver;
		}
		for (auto& p : problemCopies) {
			if (p->getScore() > bestScore) {
				bestScore = p->getScore();
				bestCandidates = p->getCandidates();
				bestStrategy = p->getPlacementStrategy();
			}
			scores.push_back(bestScore);
			placementRules.push_back(bestStrategy);
		}
		std::cout << bestCandidates.size() << "\n";
		std::cout << "iterations: " << i + batchSize << ", Best: " << bestScore << "\n";
	}

	//copy best solution to original problem
	prob->resetCandidates();
	bool scoreAdded = false;
	for (auto& cand : bestCandidates)
	{
		prob->addCandidate(cand, scoreAdded ? 0 : bestScore);
		scoreAdded = true;
	}
	prob->setAlgorithmType("h4np");
	json specifics;
	specifics["best-scores"] = scores;
	specifics["best-placement-rules"] = placementRules;
	specifics["iterations"] = iterations;
	prob->setAlgorithmSpecifics(specifics);

	return SolveStatus::Feasible;
}
