#pragma once
#include "solver.h"
#include "problem.hpp"
#include <execution>
#include "algorithms.h"

enum class PlacementRule {
    BOTTOM_LEFT,
    LEFT_BOTTOM,
    VZZ,
    HZZ,
    SPIRAL,
    ANTI_SPIRAL
};

struct IFR {
    std::vector<std::vector<bool>> raster;
    Point64 shift;
};

struct ItemWithIFR {
    Item* item;
    int quantity;
    IFR ifr;
    std::vector<ConvexEdgeList> convexDecomp;
    std::vector<ConvexEdgeList> inversePoly;
    int64_t xSpan;
    int64_t ySpan;
};

class InnerFitRaster : public Solver
{
protected:
    bool reset = false;
    int64_t xResolution;
    int64_t yResolution;
    int64_t xStepSize;
    int64_t yStepSize;
    Problem* problem;
    Polygon container;
    std::vector<Item*> items;
    std::vector<size_t> order;
    std::vector<size_t>::iterator nextIndex;

    std::vector<ItemWithIFR*> itemsWithIFR;

    void initIFRs();

    void updateNoFits(ItemWithIFR* addedPiece, Point64& translation);

    bool findNextItem(ItemWithIFR*& bestItem, Point64& translation);

    bool findBestPlacement(ItemWithIFR* testedItem, Point64& attachmentPoint);

    void addNewPiece(ItemWithIFR* item, Point64& translation);


public:
    static int64_t xRes;
    static int64_t yRes;
    InnerFitRaster() : xResolution(xRes), yResolution(yRes) {};
    InnerFitRaster(int64_t resolution): xResolution(resolution), yResolution(resolution) {};

    SolveStatus solve(Problem* prob);

    SolveStatus solve(Problem* prob, std::vector<size_t>& order);

    PlacementRule placementMode = PlacementRule::BOTTOM_LEFT;

    void setPlacementRule(int rule) { placementMode = static_cast<PlacementRule>(rule); };

    int getMaxPlacementRule() { return 5; };

    bool DEBUG = false;
    bool VERBOSE = false;
};