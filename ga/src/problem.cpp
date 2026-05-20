#include "problem.hpp"

Problem::Problem(char* file_name)
{
    std::ifstream f(file_name);

    if (f.fail())
    {
        std::cerr << "File not found!\n";
        return;
    }

    json data = json::parse(f);

    type = data["type"];
    name = data["instance_name"];
    num_items = data["num_items"];

    // n_items = data["num_items"];

    auto cont_x = data["container"]["x"];
    auto cont_y = data["container"]["y"];

    for (int i = 0; i < cont_x.size(); i++)
    {
        Point t((int)cont_x[i], (int)cont_y[i]);
        container.push_back(t);
    }

    int index = 0;

    // items = new std::vector<Item>();

    for (auto item : data["items"])
    {
        auto x = item["x"];
        auto y = item["y"];

        Polygon it;

        for (int i = 0; i < x.size(); i++)
        {
            Point t((int)x[i], (int)y[i]);
            it.push_back(t);
        }

        items.push_back(new Item(index, it, {}, item["quantity"], item["value"]));
        index++;
    }
}

void Problem::storeSolution(std::string loc)
{
    std::string vis_name = loc + "/" + name + ".json";
    std::ofstream o(vis_name);

    json output;

    output["type"] = "cgshop2024_solution";

    output["instance_name"] = name;
    output["item_indices"] = {};
    output["x_translations"] = {};
    output["y_translations"] = {};
    output["comments"] = {};
    output["score"] = score;
    output["running-time"] = time;
    output["algorithm-type"] = algorithmType;
    output["placement-strategy"] = placementStrategy;
    output["algorithm-specifics"] = algorithmSpecifics;
    output["meta-type"] = metaType;
    output["meta-specifics"] = metaSpecifics;

    for (std::string comment : comments)
    {
        output["comments"].push_back(comment);
    }

    for (Candidate item : candidates)
    {
        output["item_indices"].push_back(item.index);
        output["x_translations"].push_back((int)CGAL::to_double(item.x_translation));
        output["y_translations"].push_back((int)CGAL::to_double(item.y_translation));
        // output["x_translations"].push_back(item.x_translation.exact().numerator().doubleValue()/item.x_translation.exact().denominator().doubleValue());
        // output["y_translations"].push_back(item.y_translation.exact().numerator().doubleValue()/item.y_translation.exact().denominator().doubleValue());
    }

    o << std::setw(4) << output << std::endl;
    o.close();
}

void Problem::loadSolution(std::string file_name) {
    /*std::ifstream f(file_name);

    if (f.fail())
    {
        std::cerr << "File not found!\n";
        return;
    }

    json data = json::parse(f);

    if (name != data["instance_name"]) {
        std::cerr << "Solution does not match problem instance\n";
    }

    for (size_t i = 0; i < data["item_indices"].size(); i++)
    {
        int ind = data["item_indices"][i];

        if (ind != items[ind]->index) {
            for (size_t j = 0; j < items.size(); j++)
            {
                if (items[j]->index == ind) {
                    ind = j;
                    break;
                }
            }
        }

        addCandidate(items[ind], (long)data["x_translations"][i], (long)data["y_translations"][i]);
    }*/
}

void toIPE(std::string path, Polygon boundary, std::vector<Candidate> polygons)
{
    std::ofstream o(path);

    // Find extreme coords of the graph (if you use CGAL you can use inbuild functions instead)

    double xmin, xmax, ymin, ymax;
    for (Point p : boundary.vertices())
    {

        xmin = std::min(xmin, p.x().interval().pair().first);
        xmax = std::max(xmax, p.x().interval().pair().first);
        ymin = std::min(ymin, p.y().interval().pair().first);
        ymax = std::max(ymax, p.y().interval().pair().first);
    }
    double scale = std::max(xmax - xmin, ymax - ymin);

    // Header of the IPE File
    o << "<?xml version=\"1.0\"?>\n";
    o << "<!DOCTYPE ipe SYSTEM \"ipe.dtd\">\n";
    o << "<ipe version=\"70218\" creator=\"Ipe 7.2.24\">\n";
    o << "<info created=\"D:20221020151441\" modified=\"D:20221020151441\"/>\n";
    o << "<ipestyle name=\"basic\">\n";
    o << "<symbol name=\"mark/disk(sx)\" transformations=\"translations\">\n";
    o << "<path fill=\"sym-stroke\">\n";
    o << "0.6 0 0 0.6 0 0 e\n";
    o << "</path>\n";
    o << "</symbol>\n";
    o << "<anglesize name=\"22.5 deg\" value=\"22.5\"/>\n";
    o << "<anglesize name=\"30 deg\" value=\"30\"/>\n";
    o << "<anglesize name=\"45 deg\" value=\"45\"/>\n";
    o << "<anglesize name=\"60 deg\" value=\"60\"/>\n";
    o << "<anglesize name=\"90 deg\" value=\"90\"/>\n";
    o << "<arrowsize name=\"large\" value=\"10\"/>\n";
    o << "<arrowsize name=\"small\" value=\"5\"/>\n";
    o << "<arrowsize name=\"tiny\" value=\"3\"/>\n";

    // If you want extra colors add here, see following lines for options
    o << "<color name=\"blue\" value=\"0 0 1\"/>\n";
    o << "<color name=\"gray\" value=\"0.745\"/>\n";
    o << "<color name=\"green\" value=\"0 1 0\"/>\n";
    o << "<color name=\"red\" value=\"1 0 0\"/>\n";
    o << "<pen name=\"heavier\" value=\"0.8\"/>\n";
    o << "<pen name=\"ultrafat\" value=\"2\"/>\n";

    o << "<gridsize name=\"16 pts (~6 mm)\" value=\"16\"/>\n";
    o << "<gridsize name=\"32 pts (~12 mm)\" value=\"32\"/>\n";
    o << "<gridsize name=\"4 pts\" value=\"4\"/>\n";
    o << "<gridsize name=\"8 pts (~3 mm)\" value=\"8\"/>\n";
    o << "<opacity name=\"10%\" value=\"0.1\"/>\n";
    o << "<opacity name=\"25%\" value=\"0.25\"/>\n";
    o << "<opacity name=\"50%\" value=\"0.5\"/>\n";
    o << "<opacity name=\"75%\" value=\"0.75\"/>\n";
    o << "<symbolsize name=\"large\" value=\"5\"/>\n";
    o << "<symbolsize name=\"small\" value=\"2\"/>\n";
    o << "<symbolsize name=\"tiny\" value=\"1.1\"/>\n";
    o << "<textsize name=\"huge\" value=\"\\huge\"/>\n";
    o << "<textsize name=\"large\" value=\"\\large\"/>\n";
    o << "<textsize name=\"small\" value=\"\\small\"/>\n";
    o << "<textsize name=\"tiny\" value=\"\tiny\"/>\n";
    o << "<tiling name=\"falling\" angle=\"-60\" step=\"4\" width=\"1\"/>\n";
    o << "<tiling name=\"rising\" angle=\"30\" step=\"4\" width=\"1\"/>\n";
    o << "</ipestyle>\n";
    o << "<page>\n";
    o << "<layer name=\"boundary\"/>\n";
    o << "<layer name=\"candidates\"/>\n";
    o << "<view layers=\"boundary candidates\" active=\"candidates\"/>\n";

    o << "<path layer=\"boundary\" stroke=\"black\">";

    bool first = true;

    for (Point v : boundary.vertices())
    {
        o << ((v.x().interval().pair().first - xmin) * 560 / scale + 16) << " " << (v.y().interval().pair().first * 560 / scale + 272) << " " << (first ? "m" : "l") << "\n";
        first = false;
    }
    o << "h\n</path>\n";

    for (auto poly : polygons)
    {
        o << "<path layer=\"candidates\" stroke=\"black\" fill=\"red\" opacity=\"25%\" stroke-opacity=\"opaque\">";

        bool first = true;

        for (Point v : poly.poly.vertices())
        {
            o << ((v.x().interval().pair().first - xmin) * 560 / scale + 16) << " " << (v.y().interval().pair().first * 560 / scale + 272) << " " << (first ? "m" : "l") << "\n";
            first = false;
        }
        o << "h\n</path>\n";
    }

    o << "</page>\n";
    o << "</ipe>\n";

    o.close();

    std::string systemCom = "ipe " + path;

    int systemRet = system(systemCom.c_str());

    if (systemRet == -1)
    {
        printf("Could not open IPE");
    }
}

void Problem::visualizeSolution()
{
    std::vector<Edge> edges;
    std::vector<Point> points;

    for (auto edge : container.edges())
    {
        Point s = edge.start();
        Point t = edge.target();

        NT sx = s.x().interval().pair().first;
        NT sy = s.y().interval().pair().first;
        NT tx = t.x().interval().pair().first;
        NT ty = t.y().interval().pair().first;

        edges.push_back(Edge(Point(sx, sy), Point(tx, ty)));
        points.push_back(Point(sx, sy));
        points.push_back(Point(tx, ty));
    }

    for (Candidate candidate : candidates)
    {
        for (auto edge : candidate.poly.edges())
        {
            Point s = edge.start();
            Point t = edge.target();

            NT sx = s.x().interval().pair().first;
            NT sy = s.y().interval().pair().first;
            NT tx = t.x().interval().pair().first;
            NT ty = t.y().interval().pair().first;

            edges.push_back(Edge(Point(sx, sy), Point(tx, ty)));
            points.push_back(Point(sx, sy));
            points.push_back(Point(tx, ty));
        }
    }

    std::string path = name + ".ipe";

    toIPE(path, container, candidates);
}

void Problem::roundItems()
{
    for (Item* item : items)
    {
        Polygon it = item->poly;

        Polygon ext;
        ext.push_back(Point(-1, -1));
        ext.push_back(Point(1, -1));
        ext.push_back(Point(1, 1));
        ext.push_back(Point(-1, 1));

        Polygon sum = CGAL::minkowski_sum_2(it, ext).outer_boundary();

        // if (sum.is_clockwise_oriented()) {
        //     sum.reverse_orientation();
        // }

        item->poly = sum;
    }
}

void Problem::deleteItems(double quantity) {
    //delete items with the smalest value/area
    std::vector<int> indices(items.size());
    for (int i = 0; i < indices.size(); ++i) {
        indices[i] = i;
    }

    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return items[a]->value / items[a]->poly.area() < items[b]->value / items[b]->poly.area();
        });

    std::vector<int> removeIndices(indices.begin(), indices.begin() + std::min(static_cast<int>(items.size() * quantity), static_cast<int>(indices.size())));

    std::sort(removeIndices.begin(), removeIndices.end(), std::greater<int>());

    for (int idx : removeIndices) {
        delete items[idx];
        items.erase(items.begin() + idx);
    }
}