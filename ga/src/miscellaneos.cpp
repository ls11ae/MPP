#include "miscellaneous.h"

void parseOptions(int argc, char* argv[]) {
    cxxopts::Options options("mpp <file_name>", "Attempts to solve the MPP");

    options.add_options()
        ("h, help", "show this help message")
        ("v, visualize", "automatically open ipe at the end of the calculation")
        ("o, output", "output file location", cxxopts::value<std::string>())
        ("a, algorithm", "chooses the algorithm to use (best-fit, direct-inner-fit, inner-fit-raster)", cxxopts::value<std::string>())
        ("m, metaheuristic", "chooses the metaheuristic to use (none, genetic, h4np)", cxxopts::value<std::string>())
        ("c, config", "loads an external config file", cxxopts::value<std::string>());

    auto result = options.parse(argc, argv);

    if (result.count("help"))
    {
        std::cout << options.help();
        exit(0);
    }

    if (result.count("visualize"))
        visualize = true;

    if (result.count("output")) {
        storeSolution = true;
        outputLocation = result["output"].as<std::string>();
    }

    if (result.count("algorithm")) {
        if (result["algorithm"].as<std::string>() == "best-fit") {
            algorithm = AlgorithmType::BEST_FIT;
        }
        else if (result["algorithm"].as<std::string>() == "direct-inner-fit") {
            algorithm = AlgorithmType::DIRECT_INNER_FIT;
        }
        else if (result["algorithm"].as<std::string>() == "inner-fit-raster") {
            algorithm = AlgorithmType::INNER_FIT_RASTER;
        }
        else if (result["algorithm"].as<std::string>() == "beam-search") {
            algorithm = AlgorithmType::BEAM_SEARCH;
        }
        else {
            std::cerr << result["algorithm"].as<std::string>() << " is not a valid algorithm\n";
        }
    }

    if (result.count("metaheuristic")) {
        if (result["metaheuristic"].as<std::string>() == "none") {
            meta = Metaheuristic::NONE;
        }
        else if (result["metaheuristic"].as<std::string>() == "genetic") {
            meta = Metaheuristic::GENETIC_ALGORITHM;
        }
        else if (result["metaheuristic"].as<std::string>() == "h4np") {
            meta = Metaheuristic::H4NP;
        }
        else {
            std::cerr << result["metaheuristic"].as<std::string>() << " is not a valid metaheuristic\n";
        }
    }

    if (result.count("config")) {
        std::ifstream f(result["config"].as<std::string>());

        if (f.fail())
        {
            std::cerr << "Could not open config file " << result["config"].as<std::string>() << "\n";
        }
        else {
            json data = nlohmann::json::parse(f);
            algorithmSpecifics = data["algorithm-specifics"];
            if (data.count("meta-specifics")) {
                metaSpecifics = data["meta-specifics"];
            }
            if (data["metaheuristic"] == "genetic") {
                std::cout << "genetic\n";
                meta = Metaheuristic::GENETIC_ALGORITHM;
                json params = data["meta-specifics"];
                GAHyperparams.fromJSON(params);
            }
            else if (data["metaheuristic"] == "h4np") {
                std::cout << "H4NP\n";
                meta = Metaheuristic::H4NP;
            }


            if (data["algorithm-type"] == "best-fit") {
                algorithm = AlgorithmType::BEST_FIT;
            }
            else if (data["algorithm-type"] == "direct-inner-fit") {
                algorithm = AlgorithmType::DIRECT_INNER_FIT;
            }
            else if (data["algorithm-type"] == "inner-fit-raster") {
                algorithm = AlgorithmType::INNER_FIT_RASTER;
            }
            else if (data["algorithm-type"] == "beam-search") {
                algorithm = AlgorithmType::BEAM_SEARCH;
            }
            else {
                std::cerr << data["algorithm-type"] << " is not a valid algorithm\n";
            }

            if (data.contains("clustering")) {
                clustering = data["clustering"];
            }
            if (data.contains("delete")) {
                deleteItems = data["delete"];
            }
        }

    }
}
