#pragma once
#include "solver.h"
#include "problem.hpp"
#include "incrementalnofitsolver.h"
#include "algorithms.h"
#include <execution>


struct BeamItem {
    Item* item;
    std::vector<int> quantities;
    std::vector<Paths> innerFits;
    //Path rightEdge;
    std::vector<ConvexEdgeList> convexDecomp;
    std::vector<ConvexEdgeList> inversePoly;
    Point64 bottomLeft;
    Point64 topRight;
};

struct ItemPlacement {
    Item* item;
    Point64 translation;
};

struct FeasiblePlacement {
    std::vector<int64_t> eval;
    BeamItem* item;
    Point64 translation;
    size_t parentBeam;
};

enum class ToposAggregation {
    SUM,
    VECTOR,
    PRIORITY
};

static double combinedArea(BoundingBox& box, FeasiblePlacement& added) {
    return (std::max(box.topRight.x, added.translation.x + added.item->topRight.x) - std::min(box.bottomLeft.x, added.translation.x + added.item->bottomLeft.x))
        * (std::max(box.topRight.y, added.translation.y + added.item->topRight.y) - std::min(box.bottomLeft.y, added.translation.y + added.item->bottomLeft.y));
}

class BeamSearch : public Solver
{
protected:
    Problem* problem;
    Polygon container;
    std::vector<Item*> items;
    Path pathContainer;
    std::vector<size_t> bestFitOrder;

    Paths placedPieces;
    std::vector<std::list<BoundingBox>> placedBBoxes;

    std::vector<BeamItem*> beamItems;

    std::vector<BoundingBox> BBoxPlaced;
    std::vector<double> packedValues;

    void initIFs();

    void additionalInits() {
        Point64 bottomLeftPlaced = Point64(std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max());
        Point64 topRightPlaced = Point64(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min());
        for (size_t i = 0; i < beamWidth; i++)
        {
            BBoxPlaced.push_back({ bottomLeftPlaced, topRightPlaced, {0,0} });
        }
    };

    void updatedBBoxes(FeasiblePlacement& placement, std::list<BoundingBox>& individualBBoxes, BoundingBox& totalBBox);

    bool findBestPlacement(BeamItem* testedItem, size_t beamID, Point64& attachmentPoint, std::vector<int64_t>& eval);

    bool ItemIsBetter(BeamItem* item1, Point64& translation1, int64_t eval1, BeamItem* item2, Point64& translation2, int64_t eval2);

    bool evalIsBetter(std::vector<int64_t>& e1, std::vector<int64_t>& e2);

    ComparisonResult compareEval(int64_t first, int64_t second) {
        //returns true iff first position eval is better than second
        if (first < second)
            return ComparisonResult::BETTER;
        if (first > second)
            return ComparisonResult::WORSE;
        else
            return ComparisonResult::EQUAL;

    };

    void addNewPiece(BeamItem* item, size_t beamID, Point64& translation);

    void addSpecifics();

    void getSolutionOrder(Problem* problem, std::vector<size_t>& order);

    static void updatedInnerFit(BeamItem* item, FeasiblePlacement& addedPiece, Paths& result);


public:
    BeamSearch() {};

    SolveStatus solve(Problem* prob);

    static void getBestFitOrder(Problem* problem, std::vector<size_t>& order);

    int getMaxPlacementRule() { return 1; };
    void setPlacementRule(int rule) {
        if (rule == 0)
            placementMode = PlacementStrategy::BOTTOM_LEFT;
        else if (rule == 1)
            placementMode = PlacementStrategy::TOPOS;
    };

    size_t beamWidth = 10;
    size_t filterWidth = 3;

    ToposAggregation aggrMethod = ToposAggregation::SUM;

    int64_t scaleFactor = 1;

    PlacementStrategy placementMode = PlacementStrategy::TOPOS;

    bool reorderItems = false;

    bool DEBUG = false;
    bool VERBOSE = false;
};