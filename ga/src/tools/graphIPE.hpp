// /*
//  * This is an example header file that transforms an BGL graph into an IPE file.
//  *
//  * When you program with graphs in c++ you should probably use BGL (boost graph library) for encoding your graphs.
//  * BGL provides many algorithms that you usually use on graphs (shortest paths, flow, sorting, etc) which are well optimized.
//  * Furthermore, if you later decide to use more complex libraries (e.g. CGAL, you probably should) you don't have to modify your code as these also use boost.
//  * for more info see: https://www.boost.org/
//  *
//  * Author: Mart Hagedoorn
//  * Date: 01-03-2023
//  */

// #pragma once

// #include <utility>   // for std::pair
// #include <algorithm> // for std::for_each
// #include <fstream>
// #include <boost/graph/graph_traits.hpp>
// #include <boost/graph/adjacency_list.hpp>
// #include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
// #include <CGAL/Polygon_with_holes_2.h>
// #include <CGAL/Boolean_set_operations_2.h>
// #include <CGAL/Bbox_2.h>


// typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
// typedef CGAL::Polygon_with_holes_2<K> Polygon_with_holes;
// typedef CGAL::Polygon_2<K> Polygon;
// typedef Polygon::Vertex_iterator VertexIterator;
// typedef Polygon::Edge_const_iterator EdgeIterator;
// typedef K::Point_2 Point;
// typedef K::Segment_2 Segment;
// typedef CGAL::Bbox_2 Bbox;

// struct Edge
// {
//     Point s;
//     Point t;
// };
// // Here we asume the definition of an bidirectional graph stored in an adjacency list, modify accordingly (in your project this definition should not be here).



// void visualizeIPEFinal(std::string path, std::vector<Edge> edges, std::vector<Point> points)
// {
//     // std::string vis_name = "../Solutions/Vis/";
//     // vis_name += name;
//     // vis_name += ".ipe";

//     std::ofstream o(path);

//     // Find extreme coords of the graph (if you use CGAL you can use inbuild functions instead)

//     double xmin, xmax, ymin, ymax;
//     for (Point p : points)
//     {
//         xmin = std::min(xmin, p.x());
//         xmax = std::max(xmax, p.x());
//         ymin = std::min(ymin, p.y());
//         ymax = std::max(ymax, p.y());
//     }
//     float scale = std::max(xmax - xmin, ymax - ymin);

//     // Header of the IPE File
//     o << "<?xml version=\"1.0\"?>\n";
//     o << "<!DOCTYPE ipe SYSTEM \"ipe.dtd\">\n";
//     o << "<ipe version=\"70218\" creator=\"Ipe 7.2.24\">\n";
//     o << "<info created=\"D:20221020151441\" modified=\"D:20221020151441\"/>\n";
//     o << "<ipestyle name=\"basic\">\n";
//     o << "<symbol name=\"mark/disk(sx)\" transformations=\"translations\">\n";
//     o << "<path fill=\"sym-stroke\">\n";
//     o << "0.6 0 0 0.6 0 0 e\n";
//     o << "</path>\n";
//     o << "</symbol>\n";
//     o << "<anglesize name=\"22.5 deg\" value=\"22.5\"/>\n";
//     o << "<anglesize name=\"30 deg\" value=\"30\"/>\n";
//     o << "<anglesize name=\"45 deg\" value=\"45\"/>\n";
//     o << "<anglesize name=\"60 deg\" value=\"60\"/>\n";
//     o << "<anglesize name=\"90 deg\" value=\"90\"/>\n";
//     o << "<arrowsize name=\"large\" value=\"10\"/>\n";
//     o << "<arrowsize name=\"small\" value=\"5\"/>\n";
//     o << "<arrowsize name=\"tiny\" value=\"3\"/>\n";

//     // If you want extra colors add here, see following lines for options
//     o << "<color name=\"blue\" value=\"0 0 1\"/>\n";
//     o << "<color name=\"gray\" value=\"0.745\"/>\n";
//     o << "<color name=\"green\" value=\"0 1 0\"/>\n";
//     o << "<color name=\"red\" value=\"1 0 0\"/>\n";
//     o << "<color name=\"yellow\" value=\"1 1 0\"/>\n";

//     // Choices for thickness of lines
//     o << "<pen name=\"normal\" value=\"1\"/>\n";
//     o << "<pen name=\"fat\" value=\"1.2\"/>\n";
//     o << "<pen name=\"heavier\" value=\"0.8\"/>\n";
//     o << "<pen name=\"ultrafat\" value=\"2\"/>\n";

//     o << "<gridsize name=\"16 pts (~6 mm)\" value=\"16\"/>\n";
//     o << "<gridsize name=\"32 pts (~12 mm)\" value=\"32\"/>\n";
//     o << "<gridsize name=\"4 pts\" value=\"4\"/>\n";
//     o << "<gridsize name=\"8 pts (~3 mm)\" value=\"8\"/>\n";
//     o << "<opacity name=\"10%\" value=\"0.1\"/>\n";
//     o << "<opacity name=\"25%\" value=\"0.25\"/>\n";
//     o << "<opacity name=\"50%\" value=\"0.5\"/>\n";
//     o << "<opacity name=\"75%\" value=\"0.75\"/>\n";
//     o << "<symbolsize name=\"large\" value=\"5\"/>\n";
//     o << "<symbolsize name=\"small\" value=\"2\"/>\n";
//     o << "<symbolsize name=\"tiny\" value=\"1.1\"/>\n";
//     o << "<textsize name=\"huge\" value=\"\\huge\"/>\n";
//     o << "<textsize name=\"large\" value=\"\\large\"/>\n";
//     o << "<textsize name=\"small\" value=\"\\small\"/>\n";
//     o << "<textsize name=\"tiny\" value=\"\tiny\"/>\n";
//     o << "<tiling name=\"falling\" angle=\"-60\" step=\"4\" width=\"1\"/>\n";
//     o << "<tiling name=\"rising\" angle=\"30\" step=\"4\" width=\"1\"/>\n";
//     o << "</ipestyle>\n";
//     o << "<page>\n";
//     o << "<layer name=\"edges\"/>\n";
//     o << "<layer name=\"points\"/>\n";
//     o << "<view layers=\"edges points\" active=\"points\"/>\n";

//     for (Edge e : edges)
//     {
//         o << "<path layer=\"edges\" stroke=\"black\" pen=\"normal\">\n";
//         o << ((e.s.x() - xmin) * 560 / scale + 16) << " " << ((e.s.y() - ymin) * 560 / scale + 272) << " m\n";
//         o << ((e.t.x() - xmin) * 560 / scale + 16) << " " << ((e.t.y() - ymin) * 560 / scale + 272) << " l\n";
//         o << "h\n</path>\n";
//     }

//     // for (Point p : points)
//     // {
//     //     o << "<use layer=\"points\" name=\"mark/disk(sx)\" pos=\"" << ((p.x - xmin) * 560 / scale + 16) << " " << ((p.y - ymin) * 560 / scale + 272) << "\" size=\"normal\" stroke=\"black\"/>";
//     //     // o << "<path layer=\"polygon\" stroke=\"black\">\n";
//     //     // o << ((e.s.x - xmin) * 560 / scale + 16) << " " << ((e.s.y - ymin) * 560 / scale + 272) << " m\n";
//     //     // o << ((e.t.x - xmin) * 560 / scale + 16) << " " << ((e.t.y - ymin) * 560 / scale + 272) << " l\n";
//     //     // o << "h\n</path>\n";
//     // }

//     // for (int i = 1; i < boundary.size(); i++vis_name)
//     // {
//     //     o << ((boundary[i].x- xmin) * 560 / scale + 16) << " " << ((boundary[i].y - ymin) * 560 / scale + 272) << " l\n";
//     // }

//     // for (auto hole : holes)
//     // {
//     //     o << "<path layer=\"polygon\" stroke=\"black\" fill=\"gray\">\n";
//     //     o << ((hole[0].x - xmin) * 560 / scale + 16) << " " << ((hole[0].y - ymin) * 560 / scale + 272) << " m\n";
//     //     for (int i = 1; i < hole.size(); i++)
//     //     {
//     //         o << ((hole[i].x - xmin) * 560 / scale + 16) << " " << ((hole[i].y - ymin) * 560 / scale + 272) << " l\n";
//     //     }
//     //     o << "h\n</path>\n";
//     // }

//     // for (auto convex : cover)
//     // {
//     //     o << "<path layer=\"cover\" stroke=\"black\" fill=\"red\" opacity=\"25%\">\n";
//     //     o << ((convex[0].x - xmin) * 560 / scale + 16) << " " << ((convex[0].y - ymin) * 560 / scale + 272) << " m\n";
//     //     for (int i = 1; i < convex.size(); i++)
//     //     {
//     //         o << ((convex[i].x - xmin) * 560 / scale + 16) << " " << ((convex[i].y - ymin) * 560 / scale + 272) << " l\n";
//     //     }
//     //     o << "h\n</path>\n";
//     // }

//     o << "</page>\n";
//     o << "</ipe>\n";

//     o.close();

//     std::string systemCom = "ipe " + path;

//     int systemRet = system(systemCom.c_str());

//     if (systemRet == -1)
//     {
//         printf("Could not open IPE");
//     }
// }


// void visualizeIPE(std::vector<Polygon> items) {

//     std::vector<Edge> to_draw;
//     std::vector<Point> points;
//     int gap = 5;

//     int start = 0;
//     for (Polygon item : items) {
//         Bbox box = item.bbox();

//         for (auto edge : item.edges()) {
//             Point s = edge.start();
//             Point t = edge.start();

//             to_draw.push_back(Edge(Point(s.x() + start, s.y()), Point(t.x() + start, t.y())));
//             points.push_back(Point(s.x() + start, s.y()));
//             points.push_back(Point(t.x() + start, t.y()));
//         }

//         start += box.x_span() + gap;
//     }

//     visualizeIPEFinal("test.ipe", to_draw, points);
// }