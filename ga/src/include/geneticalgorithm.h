#pragma once

#include "solver.h"
#include "problem.hpp"
#include <execution>
#include "incrementalnofitsolver.h"
#include <random>

struct Chromosome {
	std::vector<size_t> order;
	int strategy;
	Problem* problemCopy;
	void cleanUp() { delete problemCopy; };
};

template <class T>
class GeneticAlgorithm : public Solver
{
	static_assert(std::is_base_of<Solver, T>::value, "T must be of Type Solver");
public:
    GeneticAlgorithm() {  };

	GeneticAlgorithm(GASpecifics hyperparams) {
		populationSize = hyperparams.populationSize;
		generations = hyperparams.generations;
		mutationRate = hyperparams.mutationRate;
		crossoverRate = hyperparams.crossoverRate;
		optimizeStrategy = hyperparams.optimizeStrategy;
	};

    size_t populationSize = 100;
    double mutationRate = 0.4;
    double crossoverRate = 0.5;
    size_t generations = 500;
    size_t tournamentK = 2;
    double mutateSwapRate = 0.01;
	bool optimizeStrategy = true;

    SolveStatus solve(Problem* prob);
    void init();
    void run(size_t generations);

private:
	Problem* problem;
    std::vector<Chromosome> population;
    std::vector<int64_t> fitness;
    void orderCrossover(size_t p1, size_t p2);
    void mutate(size_t individual);
	void mutateStrategy(size_t individual);
    void eval();
    void select();
	Chromosome best;
	int64_t bestFitness;
    std::vector<Item*> flatItems;
	size_t cutoffSize;
	std::vector<int64_t> bestScores;
	std::vector<std::string> placementRules = {};
	std::default_random_engine rng = std::default_random_engine{};
	std::uniform_int_distribution<> ruleDistr;
};


template<class T>
SolveStatus GeneticAlgorithm<T>::solve(Problem* prob)
{
	std::cout << prob->getString() << "\n";

	//run genetic algorithm
	problem = prob;
	init();
	run(generations);

	//copy best solution to original problem
	bool scoreAdded = false;
	for (auto cand : best.problemCopy->getCandidates())
	{
		prob->addCandidate(cand, scoreAdded? 0 : bestFitness);
		scoreAdded = true;
	}
	prob->setAlgorithmType("genetic");
	prob->setPlacementStrategy(best.problemCopy->getPlacementStrategy());
	GASpecifics specifics({ populationSize,
		generations,
		bestScores,
		mutationRate,
		crossoverRate,
		"single-swap",
		"OX" });
	prob->setMetaSpecifics(specifics);
	prob->metaSpecifics["best-placement-rules"] = placementRules;
	population.clear();
	return SolveStatus::Feasible;
}

template<class T>
void GeneticAlgorithm<T>::init() {
	int index = 0;
	std::vector<size_t> baseOrder;
	std::vector<Item*> items = problem->getItems();
	for (size_t i = 0; i < items.size(); i++)
	{
		for (size_t j = 0; j < items[i]->quantity; j++) {
			baseOrder.push_back(i);
		}
	}

	Solver* solver = static_cast<Solver*>(new T());
	ruleDistr = std::uniform_int_distribution<>(0, solver->getMaxPlacementRule());
	delete solver;

	//sorted from smallest to largest value
	auto newOrder = baseOrder;
	auto prob = problem->getCopy();
	std::sort(std::begin(newOrder), std::end(newOrder), [&items](const size_t i1, const size_t i2) {
		return (items[i1]->value < items[i2]->value);
		});
	population.push_back(Chromosome(newOrder, optimizeStrategy ? ruleDistr(rng): 0, prob));

	//sorted from largest to smallest value
	newOrder = baseOrder;
	prob = problem->getCopy();
	std::sort(std::begin(newOrder), std::end(newOrder), [&items](const size_t i1, const size_t i2) {
		return (items[i1]->value > items[i2]->value);
		});
	population.push_back(Chromosome(newOrder, optimizeStrategy ? ruleDistr(rng) : 0, prob));

	//sorted from smallest to largest area
	newOrder = baseOrder;
	prob = problem->getCopy();
	std::sort(std::begin(newOrder), std::end(newOrder), [&items](const size_t i1, const size_t i2) {
		return (items[i1]->poly.area() < items[i2]->poly.area());
		});
	population.push_back(Chromosome(newOrder, optimizeStrategy ? ruleDistr(rng) : 0, prob));

	//sorted from largest to smallest area
	newOrder = baseOrder;
	prob = problem->getCopy();
	std::sort(std::begin(newOrder), std::end(newOrder), [&items](const size_t i1, const size_t i2) {
		return (items[i1]->poly.area() > items[i2]->poly.area());
		});
	population.push_back(Chromosome(newOrder, optimizeStrategy ? ruleDistr(rng) : 0, prob));

	//sorted from largest to smallest value/area
	newOrder = baseOrder;
	prob = problem->getCopy();
	std::sort(std::begin(newOrder), std::end(newOrder), [&items](const size_t i1, const size_t i2) {
		return NT(items[i1]->value) / items[i1]->poly.area() > NT(items[i2]->value) / items[i2]->poly.area();
		});
	population.push_back(Chromosome(newOrder, optimizeStrategy ? ruleDistr(rng) : 0, prob));;

	//best fit order
	/*population.push_back(new Problem(prob->getContainer(), {}));
	population[5]->setItems(flatItems);
	IncrementalNoFitSolver::getBestFitOrder(population[4]);*/


	//initialize each individual with a random permutation of items
	for (size_t i = 5; i < populationSize; i++)
	{
		newOrder = baseOrder;
		std::shuffle(std::begin(newOrder), std::end(newOrder), rng);
		prob = problem->getCopy();
		population.push_back(Chromosome(newOrder, optimizeStrategy ? ruleDistr(rng) : 0, prob));
	}


	fitness = std::vector<int64_t>(populationSize, 0);
	eval();
}

template<class T>
void GeneticAlgorithm<T>::eval() {
	//evaluate each individual with the score of placed pieces as fitness
	size_t newBest = 0;
	
	//std::for_each(std::execution::seq, std::begin(population), std::end(population), [&](Chromosome& individual) 
#pragma omp parallel for
	for (int i = 0; i < populationSize; i++)
	{
		population[i].problemCopy->resetCandidates();
		Solver* solver = static_cast<Solver*>(new T());
		solver->setPlacementRule(population[i].strategy);
		solver->solve(population[i].problemCopy , population[i].order);
		fitness[i] = population[i].problemCopy->getScore();
		delete solver;
	}
	for (size_t i = 0; i < populationSize; i++) {
		fitness[i] = population[i].problemCopy->getScore();
		if (fitness[i] > fitness[newBest])
			newBest = i;
	}
	best = population[newBest];
	bestFitness = fitness[newBest];
	bestScores.push_back(bestFitness);
	placementRules.push_back(best.problemCopy->getPlacementStrategy());
}

template<class T>
void GeneticAlgorithm<T>::select() {
	std::vector<Chromosome> newPop = {};

	//elitism
	newPop.push_back(best);
	newPop[0].problemCopy = population[0].problemCopy;

	std::srand(std::time(0));

	//tournament selection
	for (size_t i = 1; i < populationSize; i++) {
		size_t rndInd = std::rand() % populationSize;
		size_t tournamentBest = rndInd;
		double fit = fitness[rndInd];
		for (int j = 0; j < tournamentK; j++) {
			int contInd = std::rand() % populationSize;
			double contFit = fitness[contInd];
			if (contFit > fit) {
				tournamentBest = contInd;
				fit = contFit;
			}
		}
		newPop.push_back(population[tournamentBest]);
		newPop[i].problemCopy = population[i].problemCopy;
	}
	population = newPop;
}

template<class T>
void GeneticAlgorithm<T>::mutate(size_t chrom) {
	//mutate genome by randomly swapping a number of polygons
	size_t numMutations = 1;//std::max((int)(mutateSwapRate * prob->getNumItems()), 1);
	for (size_t i = 0; i < numMutations; i++)
	{
		std::swap(population[chrom].order[rand() % population[chrom].order.size()], population[chrom].order[rand() % population[chrom].order.size()]);
	}
}

template<class T>
void GeneticAlgorithm<T>::mutateStrategy(size_t chrom) {
	//mutate genome by changing packing strategy
	population[chrom].strategy = ruleDistr(rng);
}

template<class T>
void GeneticAlgorithm<T>::orderCrossover(size_t p1, size_t p2) {
	std::vector<size_t> c1Order(population[p1].order.size(), std::numeric_limits<size_t>::max());
	std::vector<size_t> c2Order(population[p2].order.size(), std::numeric_limits<size_t>::max());

	//items missing in child1 or 2 after copying segments
	std::vector<size_t> missing1;
	std::vector<size_t> missing2;

	size_t crossoverPoints[4];
	crossoverPoints[0] = 0;
	for (size_t i = 1; i < sizeof(crossoverPoints) / sizeof(*crossoverPoints) - 1; i++)
	{
		crossoverPoints[i] = rand() % population[p1].order.size();
	}
	crossoverPoints[3] = population[p1].order.size();
	std::sort(std::begin(crossoverPoints), std::end(crossoverPoints));

	//copy items of p1 into c1 and p2 into c2 ich randomly chosen segments
	for (size_t i = 0; i < sizeof(crossoverPoints) / sizeof(*crossoverPoints) - 1; i++)
	{
		for (size_t j = crossoverPoints[i]; j < crossoverPoints[i + 1]; j++)
		{
			if (i % 2 == 0) {
				c1Order[j] = population[p1].order[j];
				c2Order[j] = population[p2].order[j];
			}
			else
			{
				missing1.push_back(population[p1].order[j]);
				missing2.push_back(population[p2].order[j]);
			}
		}
	}
	//fill gaps in child1 with missing items in order of p2
	size_t nextInsert = crossoverPoints[1];
	for (auto item : population[p2].order)
	{
		auto iter = std::find(missing1.begin(), missing1.end(), item);
		if (iter != missing1.end()) {
			missing1.erase(iter);
			c1Order[nextInsert] = item;
			while (nextInsert < c1Order.size() && c1Order[nextInsert] != std::numeric_limits<size_t>::max()) {
				++nextInsert;
			}
			if (nextInsert >= c1Order.size())
				break;
		}
	}

	//fill gaps in child2 with missing items in order of p1
	nextInsert = crossoverPoints[1];
	for (auto item : population[p1].order)
	{
		auto iter = std::find(missing2.begin(), missing2.end(), item);
		if (iter != missing2.end()) {
			missing2.erase(iter);
			c2Order[nextInsert] = item;
			while (nextInsert < c2Order.size() && c2Order[nextInsert] != std::numeric_limits<size_t>::max()) {
				++nextInsert;
			}
			if (nextInsert >= c2Order.size())
				break;
		}
	}
	population[p1].order = c1Order;
	population[p2].order = c2Order;
}

template<class T>
void GeneticAlgorithm<T>::run(size_t generations) {
	std::srand(std::time(0));
	std::cout << "Generation 0 done, best score: " << bestFitness << std::endl;
	for (size_t i = 1; i <= generations-1; i++)
	{
		select();
		
		for (size_t j = 1; j < populationSize; j++)
		{

			//crossover
			if (2 * j + 1 < populationSize && (double)std::rand() / RAND_MAX < crossoverRate) {
				orderCrossover(2 * j, 2 * j + 1);
			}

			//mutation
			if ((double)std::rand() / RAND_MAX < mutationRate) {
				mutate(j);
			}

			//mutation
			if ((double)std::rand() / RAND_MAX < mutationRate) {
				mutateStrategy(j);
			}
		}
		
		eval();
		std::cout << "Generation " << i << " done, best score: " << bestFitness << std::endl;
	}
}
