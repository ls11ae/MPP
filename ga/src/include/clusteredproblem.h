#pragma once

#include "problem.hpp"

struct ClusterCandidates {
	size_t item1;
	size_t item2;
	Point dominantPoint;
	NT eval;
	Polygon poly;
};

struct Cluster : public Item
{
	Cluster(int index, Polygon poly, std::vector<Polygon> inners, int quantity, int value, std::vector<Item*> packedItems, std::vector<NT> x_translations, std::vector<NT> y_translations) :
		Item{ index, poly, inners, quantity, value }, packedItems{ packedItems }, x_translations{ x_translations }, y_translations{ y_translations } {}
	std::vector<Item*> packedItems;
	std::vector<NT> x_translations;
	std::vector<NT> y_translations;
};

class ClusteredProblem : public Problem {
protected:
	std::vector<Item*> originalItems;
	std::vector<Cluster*> clusters;
	void cluster();
	bool attachPoly(Item* fixed, Item* movable, Point& dominantPoint);

	static void getHullVacancies(const Polygon& poly, std::vector<Polygon>& vacancies);

public:
	double percentage = 0.1;
	//ClusteredProblem(&Problem prob);
	ClusteredProblem(char* file_name);
	ClusteredProblem(char* file_name, double minFit);
	ClusteredProblem(Problem* toCopy): Problem(toCopy) {};
	virtual void addCandidate(Item* item, NT x_translation, NT y_translation) override;
	virtual Problem* getCopy() override {
		return new ClusteredProblem(this);
	};
};