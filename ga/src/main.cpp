#include "main.h"
#include <execution>

#define tms std::chrono::high_resolution_clock::now()
#define dif(a, b) std::chrono::duration_cast<std::chrono::milliseconds>(a - b)

int main(int argc, char **argv)
{
    /*auto p = new Problem(argv[1]);
    p->loadSolution(std::string("D:\\uni\\MA\\experiments\\BEST_FIT_MIN_DIST34976016\\jigsaw_rcf4_x296c58c_70.json"));
    p->visualizeSolution();
    return 0;*/

    parseOptions(argc, argv);
    //problem->roundItems();

    Problem* problem;
    if (clustering > 0.0)
        problem = new ClusteredProblem(argv[1], clustering);
    else
        problem = new Problem(argv[1]);
    Solver* solver;

    if (deleteItems > 0.0)
        problem->deleteItems(deleteItems);

    problem->setAlgorithmSpecifics(algorithmSpecifics);
    problem->setMetaSpecifics(metaSpecifics);

    if (algorithm == AlgorithmType::INNER_FIT_RASTER) {
        if (algorithmSpecifics.contains("resolution")) {
            InnerFitRaster::xRes = algorithmSpecifics["resolution"];
            InnerFitRaster::yRes = algorithmSpecifics["resolution"];
        }
    }

    switch (meta)
    {
    case Metaheuristic::NONE:
        switch (algorithm)
        {
        case AlgorithmType::DIRECT_INNER_FIT:
        {
            problem->setAlgorithmType("direct-inner-fit");
            auto direct = new IncrementalNoFitSolver();
            if (algorithmSpecifics["strategy"] == "bottom-left")
                direct->placementMode = PlacementStrategy::BOTTOM_LEFT;
            else if (algorithmSpecifics["strategy"] == "bottom-right")
                direct->placementMode = PlacementStrategy::BOTTOM_RIGHT;
            else if (algorithmSpecifics["strategy"] == "top-left")
                direct->placementMode = PlacementStrategy::TOP_LEFT;
            else if (algorithmSpecifics["strategy"] == "top-right")
                direct->placementMode = PlacementStrategy::TOP_RIGHT;
            else if (algorithmSpecifics["strategy"] == "min-dist")
                direct->placementMode = PlacementStrategy::MIN_DIST;
            else if (algorithmSpecifics["strategy"] == "topos")
                direct->placementMode = PlacementStrategy::TOPOS;
            solver = static_cast<Solver*>(direct);

            break;
        }
        case AlgorithmType::INNER_FIT_RASTER:
        {
            problem->setAlgorithmType("inner-fit-raster");
            auto ifr = new InnerFitRaster();
            if (algorithmSpecifics["strategy"] == "bottom-left")
                ifr->placementMode = PlacementRule::BOTTOM_LEFT;
            else if (algorithmSpecifics["strategy"] == "left-bottom")
                ifr->placementMode = PlacementRule::LEFT_BOTTOM;
            else if (algorithmSpecifics["strategy"] == "vzz")
                ifr->placementMode = PlacementRule::VZZ;
            else if (algorithmSpecifics["strategy"] == "hzz")
                ifr->placementMode = PlacementRule::HZZ;
            else if (algorithmSpecifics["strategy"] == "spiral")
                ifr->placementMode = PlacementRule::SPIRAL;
            else if (algorithmSpecifics["strategy"] == "anti-spiral")
                ifr->placementMode = PlacementRule::ANTI_SPIRAL;
            solver = static_cast<Solver*>(ifr);
            break;
        }
        case AlgorithmType::BEST_FIT:
        {
            problem->setAlgorithmType("best-fit");
            auto direct = new IncrementalNoFitSolver();
            if (algorithmSpecifics["strategy"] == "bottom-left")
                direct->placementMode = PlacementStrategy::BOTTOM_LEFT;
            else if (algorithmSpecifics["strategy"] == "bottom-right")
                direct->placementMode = PlacementStrategy::BOTTOM_RIGHT;
            else if (algorithmSpecifics["strategy"] == "top-left")
                direct->placementMode = PlacementStrategy::TOP_LEFT;
            else if (algorithmSpecifics["strategy"] == "top-right")
                direct->placementMode = PlacementStrategy::TOP_RIGHT;
            else if (algorithmSpecifics["strategy"] == "min-dist")
                direct->placementMode = PlacementStrategy::MIN_DIST;
            else if (algorithmSpecifics["strategy"] == "topos")
                direct->placementMode = PlacementStrategy::TOPOS;
            direct->bestFit = true;
            solver = static_cast<Solver*>(direct);
            break;
        }
        case AlgorithmType::BEAM_SEARCH:
        {
            problem->setAlgorithmType("beam-search");
            auto beam = new BeamSearch();
            if (algorithmSpecifics["strategy"] == "bottom-left")
                beam->placementMode = PlacementStrategy::BOTTOM_LEFT;
            else if (algorithmSpecifics["strategy"] == "bottom-right")
                beam->placementMode = PlacementStrategy::BOTTOM_RIGHT;
            else if (algorithmSpecifics["strategy"] == "top-left")
                beam->placementMode = PlacementStrategy::TOP_LEFT;
            else if (algorithmSpecifics["strategy"] == "top-right")
                beam->placementMode = PlacementStrategy::TOP_RIGHT;
            else if (algorithmSpecifics["strategy"] == "min-dist")
                beam->placementMode = PlacementStrategy::MIN_DIST;
            else if (algorithmSpecifics["strategy"] == "topos")
                beam->placementMode = PlacementStrategy::TOPOS;

            if (algorithmSpecifics.contains("beam-width"))
                beam->beamWidth = algorithmSpecifics["beam-width"];
            if (algorithmSpecifics.contains("filter-width"))
                beam->filterWidth = algorithmSpecifics["filter-width"];

            solver = static_cast<Solver*>(beam);
            break;
        }
        default:
            break;
        }
        break;
    case Metaheuristic::GENETIC_ALGORITHM:
        problem->setMetaType("genetic");
        switch (algorithm)
        {
        case AlgorithmType::DIRECT_INNER_FIT:
            solver = static_cast<Solver*>(new GeneticAlgorithm<IncrementalNoFitSolver>(GAHyperparams));
            break;
        case AlgorithmType::INNER_FIT_RASTER:
            solver = static_cast<Solver*>(new GeneticAlgorithm<InnerFitRaster>(GAHyperparams));
            break;
        default:
            std::cerr << "Algorithm not compatible with metaheuristic\n";
            break;
        }
        break;
    case Metaheuristic::H4NP: {
        problem->setMetaType("h4np");
        size_t iterations = 100;
        if (metaSpecifics.contains("iterations"))
            iterations = metaSpecifics["iterations"];
        switch (algorithm)
        {
        case AlgorithmType::DIRECT_INNER_FIT:
            solver = static_cast<Solver*>(new H4NP<IncrementalNoFitSolver>(iterations));
            break;
        case AlgorithmType::INNER_FIT_RASTER:
            solver = static_cast<Solver*>(new H4NP<InnerFitRaster>(iterations));
            break;
        default:
            std::cerr << "Algorithm not compatible with metaheuristic\n";
            break;
        }
        break;
    }
    default:
        break;
    }
    
    auto start = tms;
    solver->solve(problem);
    auto time = dif(tms, start);

    delete solver;

    problem->setRunningTime(time.count());

    problem->prettyPrint();
    if(storeSolution)
        problem->storeSolution(outputLocation);

    if (visualize)
        problem->visualizeSolution();

    delete (problem);


    return 0;
}