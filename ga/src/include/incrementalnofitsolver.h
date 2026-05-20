#pragma once
#include "solver.h"
#include "problem.hpp"
#include <execution>
#include "algorithms.h"

static void intersectZCallback(const Point64& e1top, const Point64& e1bot,
    const Point64& e2top, const Point64& e2bot, Point64& pt)
{
    const Point64 *first, *second;

    //byte map storing direction point was shifted in - left, right, up, down
    unsigned char shift = 0;
    if (e1top.z < 0 || e1bot.z < 0) {
        if (e2bot.y > e2top.y) {
            pt.z = std::min(e2bot.z, e2top.z);
            return;
        }
        pt.z = std::min(e1bot.z, e1top.z);
        return;
    }
    if (e1top.z == e1bot.z + 1) {
        first = &e1bot;
        second = &e1top;
    }
    else if (e1bot.z == e1top.z + 1) {
        first = &e1top;
        second = &e1bot;
    }
    else if (e1top.z < e1bot.z) {
        first = &e1bot;
        second = &e1top;
    }
    else
    {
        first = &e1top;
        second = &e1bot;
    }
    if (first->y < second->y) {
        shift += 0b1000;
        //pt.x -= 1;
        if (first->x < second->x) {
            //pt.y += 1;
            shift += 0b0010;
        }
        else if (first->x > second->x) {
            //pt.y -= 1;
            shift += 0b0001;
        }
    }
    else if (first->y > second->y) {
        shift += 0b0100;
        //pt.x += 1;
        if (first->x < second->x) {
            //pt.y += 1;
            shift += 0b0010;
        }
        else if (first->x > second->x) {
            //pt.y -= 1;
            shift += 0b0001;
        }
    }
    else {
        pt.y = first->y;
    }
    if (pt.x <= e2top.x && e2bot.x > e2top.x ||
        pt.x >= e2top.x && e2bot.x < e2top.x ||
        pt.x <= e2bot.x && e2bot.x < e2top.x ||
        pt.x >= e2bot.x && e2bot.x > e2top.x) {
        shift += 0b10000;
    }
    pt.z = -shift;
}

static int64_t approxDistance(int64_t dx, int64_t dy)
{
    int64_t min, max, approx;

    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    if (dx < dy)
    {
        min = dx;
        max = dy;
    }
    else {
        min = dy;
        max = dx;
    }

    approx = (max * 1007) + (min * 441);
    if (max < (min << 4))
        approx -= (max * 40);

    // add 512 for proper rounding
    return ((approx + 512) >> 10);
}

enum class PlacementStrategy {
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    TOP_LEFT,
    TOP_RIGHT,
    MIN_DIST,
    CONCAVE_FIT,
    TOPOS
};

enum class ComparisonResult {
    BETTER,
    WORSE,
    EQUAL
};

struct ItemWithNoFit {
    Item* item;
    int quantity;
    Paths innerFit;
    Path rightEdge;
    std::vector<ConvexEdgeList> convexDecomp;
    std::vector<ConvexEdgeList> inversePoly;
    Point64 bottomLeft;
    Point64 topRight;
};

struct BoundingBox {
    Point64 bottomLeft;
    Point64 topRight;
    Point64 center;
};

class IncrementalNoFitSolver : public Solver
{
protected:
    Problem* problem;
    Polygon container;
    std::vector<Item*> items;
    Path pathContainer;
    std::vector<size_t> bestFitOrder;

    Paths placedPieces;
    std::list<BoundingBox> placedBBoxes;

    std::vector<ItemWithNoFit*> itemsWithNoFit;

    BoundingBox BBoxPlaced;

    void initNoFits(size_t index);

    void additionalInits() {
        Point64 bottomLeftPlaced = Point64(std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max());
        Point64 topRightPlaced = Point64(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min());
        BBoxPlaced = { bottomLeftPlaced, topRightPlaced, {0,0} };
    };

    void updateNoFits(ItemWithNoFit* addedPiece, Point64& translation);

    void additionalUpdates(ItemWithNoFit* addedPiece, Point64& translation);

    bool findBestItem(ItemWithNoFit* &bestItem, Point64& translation);

    bool findBestPlacement(ItemWithNoFit* testedItem, Point64& attachmentPoint, int64_t& eval);

    bool ItemIsBetter(ItemWithNoFit* item1, Point64& translation1, int64_t eval1, ItemWithNoFit* item2, Point64& translation2, int64_t eval2);

    ComparisonResult compareEval(int64_t first, int64_t second) {
        //returns true iff first position eval is better than second
        if (first < second)
            return ComparisonResult::BETTER;
        if (first > second)
            return ComparisonResult::WORSE;
        else
            return ComparisonResult::EQUAL;

    };

    void addNewPiece(ItemWithNoFit* item, Point64& translation);

    void addSpecifics();

    void getSolutionOrder(Problem* problem, std::vector<size_t>& order);


public:
    IncrementalNoFitSolver() {};

    SolveStatus solve(Problem* prob, std::vector<size_t>& order);

    SolveStatus solve(Problem* prob);

    static void getBestFitOrder(Problem* problem, std::vector<size_t>& order);

    int getMaxPlacementRule() { return 1; };
    void setPlacementRule(int rule) {
        if (rule == 0)
            placementMode = PlacementStrategy::BOTTOM_LEFT;
        else if (rule == 1)
            placementMode = PlacementStrategy::TOPOS;
    };

    bool bestFit = false;

    size_t batchSize = 999999;

    int64_t scaleFactor = 1;

    PlacementStrategy placementMode = PlacementStrategy::BOTTOM_LEFT;
   
    bool reorderItems = false;

    bool DEBUG = false;
    bool VERBOSE = false;
};