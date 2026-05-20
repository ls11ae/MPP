#include "incrementalnofitsolver.h"

void pathsToIPE(std::string path, Path boundary, std::vector<Path> polygons)
{
	std::ofstream o(path);

	// Find extreme coords of the graph (if you use CGAL you can use inbuild functions instead)

	int64_t xmin=999999999, xmax=-999999999, ymin = 9999999999, ymax=-999999999;
	for (auto p : boundary)
	{

		xmin = std::min(xmin, p.x);
		xmax = std::max(xmax, p.x);
		ymin = std::min(ymin, p.y);
		ymax = std::max(ymax, p.y);
	}
	for (auto poly : polygons)
	{
		for (auto p : poly)
		{

			xmin = std::min(xmin, p.x);
			xmax = std::max(xmax, p.x);
			ymin = std::min(ymin, p.y);
			ymax = std::max(ymax, p.y);
		}
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
	o << "<layer name=\"points\"/>\n";
	o << "<view layers=\"boundary candidates points\" active=\"candidates\"/>\n";

	o << "<path layer=\"boundary\" stroke=\"black\">";

	bool first = true;

	for (auto v : boundary)
	{
		o << (((double) (v.x - xmin) * 560) / scale + 16) << " " << (((double)v.y * 560) / scale + 272) << " " << (first ? "m" : "l") << "\n";
		first = false;
	}
	o << "h\n</path>\n";

	for (auto poly : polygons)
	{
		o << "<path layer=\"candidates\" stroke=\"black\" fill=\"red\" opacity=\"25%\" stroke-opacity=\"opaque\">";

		bool first = true;

		for (auto v : poly)
		{
			o << (((double)(v.x - xmin) * 560) / scale + 16) << " " << (((double) v.y * 560) / scale + 272) << " " << (first ? "m" : "l") << "\n";
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

void pathsToIPE(std::string path, Path boundary, std::vector<Paths> polygons) {
	std::vector<Path> flattened;
	for (auto polys : polygons) {
		for (auto poly : polys) {
			flattened.push_back(poly);
		}
	}
	pathsToIPE(path, boundary, flattened);
}

void toIPE2(std::string path, Polygon boundary, std::vector<std::vector<Polygon>> polygons, std::vector<Paths> paths, int64_t scaleFactor = 1)
{
	std::vector<Path> flattened;
	for (auto polys : paths) {
		for (auto poly : polys) {
			flattened.push_back(poly);
		}
	}
	for (auto polys : polygons) {
		for (auto poly : polys) {
			Path newPath;
			for (auto p : poly)
			{
				newPath.push_back({ p.x().interval().sup() * scaleFactor, p.y().interval().sup() * scaleFactor });
			}
			flattened.push_back(newPath);
		}
	}
	Path boundaryPath;
	for (auto p : boundary)
	{
		boundaryPath.push_back({ p.x().interval().sup() * scaleFactor, p.y().interval().sup() * scaleFactor });
	}
	pathsToIPE(path, boundaryPath, flattened);
}

SolveStatus IncrementalNoFitSolver::solve(Problem* prob, std::vector<size_t>& order) {
	problem = prob;
	int c = 0;

	initNoFits(0);
	additionalInits();

	for (auto i: order) {
		Point64 translation;
		int64_t eval;

		if (!findBestPlacement(itemsWithNoFit[i], translation, eval)) {
			continue;
		}
		
		--itemsWithNoFit[i]->quantity;
		if (itemsWithNoFit[i]->quantity < 0) {
			return SolveStatus::Unsolved;
		}

		addNewPiece(itemsWithNoFit[i], translation);
		++c;
		if (VERBOSE && (c % 100 == 0)) {
			std::cout << problem->getString() << ": " << c << " pieces placed\n";
		}
	}
	addSpecifics();

	while (!itemsWithNoFit.empty()) delete itemsWithNoFit.back(), itemsWithNoFit.pop_back();
	return SolveStatus::Feasible;
}

SolveStatus IncrementalNoFitSolver::solve(Problem* prob)
{
	problem = prob;
	int c = 0;

	int iters = 0;
	for(size_t i = 0; i < problem->getNumItems(); i += batchSize) {
		initNoFits(i);
		additionalInits();
		while (true)
		{
			ItemWithNoFit* bestItem = NULL;
			Point64 translation;

			if (!findBestItem(bestItem, translation)) {
				break;
			}
			
			if (reorderItems) {
				bestFitOrder.push_back(bestItem->item->index);
			}

			--bestItem->quantity;

			addNewPiece(bestItem, translation);

			++c;
			if (VERBOSE && (c % 100 == 0)) {
				std::cout << problem->getString() << ": " << c << " pieces placed\n";
			}
		}
		iters++;
		//toIPE2("test.ipe", problem->getContainer(), { placedPieces }, {});
	}
	if (reorderItems) {
		for (auto it : itemsWithNoFit) {
			for (size_t i = 0; i < it->quantity; i++)
			{
				bestFitOrder.push_back(it->item->index);
			}
		}
	}
	
	addSpecifics();

	while (!itemsWithNoFit.empty()) delete itemsWithNoFit.back(), itemsWithNoFit.pop_back();
	return SolveStatus::Feasible;
}

void IncrementalNoFitSolver::addSpecifics() {
	switch (placementMode)
	{
	case PlacementStrategy::BOTTOM_LEFT:
		problem->setPlacementStrategy("bottom-left");
		problem->addComment("Position ist determined by left-most of the bottom-most positions");
		break;
	case PlacementStrategy::BOTTOM_RIGHT:
		break;
	case PlacementStrategy::TOP_LEFT:
		break;
	case PlacementStrategy::TOP_RIGHT:
		break;
	case PlacementStrategy::MIN_DIST:
		problem->setPlacementStrategy("min-dist");
		problem->addComment("Closest position to (0,0) ist chosen (euclidean distance)");
		break;
	case PlacementStrategy::CONCAVE_FIT:
		break;
	case PlacementStrategy::TOPOS:
		problem->setPlacementStrategy("topos");
		break;
	default:
		break;
	}
}

void IncrementalNoFitSolver::initNoFits(size_t index) {
	//Initialize no-fit polygon for each item as polygon around container with a hole for each free space

	if (index == 0) {
		container = problem->getContainer();

		for (auto& v : container) {
			pathContainer.push_back({ v.x().interval().sup() * scaleFactor, v.y().interval().sup() *scaleFactor });
		}

		int wall = 10e6;

		//build new polygon as outside of container

		auto bottom = container.bottom_vertex();
		auto top = container.top_vertex();

		container.reverse_orientation();
		auto containerStart = CGAL::Circulator_from_iterator(container.begin(), container.end(), container.top_vertex());
		auto containerEnd = containerStart - 1;

		auto dist = CGAL::squared_distance(*containerStart, *containerEnd);
		containerEnd = CGAL::Circulator_from_iterator(container.begin(), container.end(), container.insert(containerStart.current_iterator(),
			{ containerStart->x() + (containerEnd->x() - containerStart->x()) / dist, containerStart->y() + (containerEnd->y() - containerStart->y()) / dist }));
		containerStart = containerEnd + 1;

		auto it = container.insert(containerStart.current_iterator(), { containerStart->x(), containerStart->y() + wall });
		it = container.insert(it, { container.right_vertex()->x() + wall, it->y() });
		it = container.insert(it, { it->x(), container.bottom_vertex()->y() - wall });
		it = container.insert(it, { container.left_vertex()->x() - wall, it->y() });
		it = container.insert(it, { it->x(), container.top_vertex()->y() });

		items = problem->getItems();

	}

	Polygon placedPoly;
	for (auto& placedPart : placedPieces) {
		for (auto& p : placedPart) {
			placedPoly.push_back({ p.x / scaleFactor, p.y / scaleFactor });
		}
	}
	size_t end = std::min(items.size(), index + batchSize);

	itemsWithNoFit = std::vector<ItemWithNoFit*>(items.size(), nullptr);
	#pragma omp parallel for
	for (int i = index; i < end; i++) {
		Polygon inverse;
		for (size_t j = 0; j < items[i]->poly.vertices().size(); j++)
		{
			inverse.push_back({ -items[i]->poly[j].x(), -items[i]->poly[j].y() });
		}

		std::vector<Point> points = items[i]->poly.vertices();
		std::list<Partition_traits::Polygon_2> polyDecompIndices;

		convexDecompositionIndices(points, polyDecompIndices);

		std::vector<ConvexEdgeList> polyDecomp;
		std::vector<ConvexEdgeList> inverseDecomp;

		edgeListsFromDecomposition(points, polyDecompIndices, polyDecomp, inverseDecomp);

		//create path from right edge of polygon for insertion when placed
		Path rightEdge = { Point64(int64_t(-100), (int64_t)items[i]->poly.bottom_vertex()->y().interval().sup() * scaleFactor) };
		auto rightIt = CGAL::Circulator_from_iterator(items[i]->poly.begin(), items[i]->poly.end(), items[i]->poly.bottom_vertex());
		while ((*rightIt).y() == items[i]->poly.bottom_vertex()->y()) {
			++rightIt;
		}
		--rightIt;
		--rightIt;
		do {
			++rightIt;
			rightEdge.push_back(Point64((*rightIt).x().interval().sup() * scaleFactor, (*rightIt).y().interval().sup() * scaleFactor));
		} while ((*rightIt).y() < items[i]->poly.top_vertex()->y());
		rightEdge.push_back(Point64(int64_t(-100), (int64_t)items[i]->poly.top_vertex()->y().interval().sup() * scaleFactor));

		//pathsToIPE("test.ipe", container, { inverseNext });
		auto innerFitHoles = CGAL::minkowski_sum_2(container, inverse).holes();
		Paths innerFit;

		for (auto innerFitHole: innerFitHoles) {
			innerFitHole.reverse_orientation();
			Path hole;
			for (auto p: innerFitHole)
			{
				Point64 clipperP(p.x().interval().sup(), p.y().interval().sup(), 1);
				hole.push_back({ CGAL::to_double(p.x()) * scaleFactor, CGAL::to_double(p.y()) * scaleFactor, 999 });
			}
			innerFit.push_back(Clipper2Lib::TrimCollinear(hole));
		}
		//innerFit = Clipper2Lib::InflatePaths(innerFit, -12, Clipper2Lib::JoinType::Square, Clipper2Lib::EndType::Polygon);


		Polygon placedNoFit = CGAL::minkowski_sum_2(placedPoly, inverse).outer_boundary();
		Path placedNoFitPath;
		for (auto& p : placedNoFit) {
			placedNoFitPath.push_back({ p.x().interval().sup() * scaleFactor, p.y().interval().sup() * scaleFactor });
		}
		innerFit = Clipper2Lib::Difference(innerFit, { placedNoFitPath }, Clipper2Lib::FillRule::NonZero);

		for (auto& path : innerFit) {
			int64_t index = 0;
			for (auto& p : path)
			{
				p.z = index;
				++index;
			}
		}

		//find bottom-left and top-right of bounding box
		int64_t minX = std::numeric_limits<int64_t>::max();
		int64_t minY = std::numeric_limits<int64_t>::max();
		int64_t maxX = std::numeric_limits<int64_t>::min();
		int64_t maxY = std::numeric_limits<int64_t>::min();

		for (auto& p : items[i]->poly) {
			int64_t currentX = CGAL::to_double(p.x());
			int64_t currentY = CGAL::to_double(p.y());
			if (currentX < minX)
				minX = currentX;
			if (currentY < minY)
				minY = currentY;
			if (currentX > maxX)
				maxX = currentX;
			if (currentY > maxY)
				maxY = currentY;
		}
		Point64 bl = { minX, minY };
		Point64 tr = { maxX,maxY };

		itemsWithNoFit[i] = new ItemWithNoFit({ items[i], items[i]->quantity, innerFit, rightEdge, polyDecomp, inverseDecomp, bl, tr });
	}
}

void IncrementalNoFitSolver::additionalUpdates(ItemWithNoFit* addedPiece, Point64& translation) {
	//store approximation of added pieces
	Path added;
	for (auto& p : addedPiece->rightEdge) {
		added.push_back(p + translation);
	}
	added[0].x = -100000;
	added[added.size()-1].x = -100000;
	placedPieces = Clipper2Lib::Union(placedPieces, { added }, Clipper2Lib::FillRule::NonZero);
	if (placedPieces.size() > 1) {
		size_t max = 0;
		Path* outer;
		for (auto& part : placedPieces) {
			if (part.size() > max) {
				max = part.size();
				outer = &part;
			}
		}
		placedPieces = { *outer };
	}

	//add new bbox
	BoundingBox newBBox = { addedPiece->bottomLeft + translation, addedPiece->topRight + translation, translation +  (addedPiece->topRight + addedPiece->bottomLeft) * 0.5 };
	placedBBoxes.push_back(newBBox);
	if (placedBBoxes.size() > 50)
		placedBBoxes.pop_front();

	//update placed bbox
	if (newBBox.bottomLeft.x < BBoxPlaced.bottomLeft.x)
		BBoxPlaced.bottomLeft.x = newBBox.bottomLeft.x;
	if (newBBox.bottomLeft.y < BBoxPlaced.bottomLeft.y)
		BBoxPlaced.bottomLeft.y = newBBox.bottomLeft.y;
	if (newBBox.topRight.x > BBoxPlaced.topRight.x)
		BBoxPlaced.topRight.x = newBBox.topRight.x;
	if (newBBox.topRight.y > BBoxPlaced.topRight.y)
		BBoxPlaced.topRight.y = newBBox.topRight.y;
	BBoxPlaced.center = BBoxPlaced.bottomLeft * 0.5 + BBoxPlaced.topRight * 0.5;

}

void IncrementalNoFitSolver::updateNoFits(ItemWithNoFit* addedPiece, Point64& translation) {

	//update no-fit polygons
	
	//long t1 = 0, t2 = 0;
	//std::for_each(std::execution::par, std::begin(itemsWithNoFit), std::end(itemsWithNoFit), [&](ItemWithNoFit *it)
	#pragma omp parallel for
	for (int i = 0; i < itemsWithNoFit.size(); i++)
	{
		//auto start = std::chrono::high_resolution_clock::now();
		Paths noFitParts = minkowskiSum(addedPiece->convexDecomp, itemsWithNoFit[i]->inversePoly, translation);
		//auto noFit = Clipper2Lib::Union(noFitParts, Clipper2Lib::FillRule::NonZero);
		//noFit = Clipper2Lib::InflatePaths(noFit, 1, Clipper2Lib::JoinType::Miter, Clipper2Lib::EndType::Polygon);

		/*for (size_t part = 0; part < noFit.size(); part++) {
			auto previous = noFit[part].end() - 1;
			for (auto p = noFit[part].begin(); p != noFit[part].end(); p++) {
				if (std::abs(previous->x - p->x) + std::abs(previous->y - p->y) <= 1) {
					p = noFit[part].erase(previous);
					if (p != noFit[part].begin()) {
						previous = p - 1;
					}
					else {
						previous = noFit[part].end() - 1;
					}
				}
				previous = p;
			}
		}*/

		auto noFit = Clipper2Lib::InflatePaths(noFitParts, 1, Clipper2Lib::JoinType::Square, Clipper2Lib::EndType::Polygon);

		Clipper2Lib::Clipper64 c;
		c.AddSubject(Clipper2Lib::InflatePaths(itemsWithNoFit[i]->innerFit, -1, Clipper2Lib::JoinType::Miter, Clipper2Lib::EndType::Polygon));
		c.AddClip(noFit);
		Paths result;

		c.Execute(Clipper2Lib::ClipType::Difference, Clipper2Lib::FillRule::EvenOdd, result);

		//auto subject = Clipper2Lib::InflatePaths(itemsWithNoFit[i]->innerFit, -1, Clipper2Lib::JoinType::Miter, Clipper2Lib::EndType::Polygon);
		//auto result = Clipper2Lib::Difference(itemsWithNoFit[i]->innerFit, noFit, Clipper2Lib::FillRule::EvenOdd);

		/*for (size_t path = 0; path < result.size(); path++)
		{
			int64_t index = 0;
			auto prev = result[path].end() - 1;
			bool deleteNext = false;
			for (auto p = result[path].begin(); p != result[path].end(); p++)
			{
				if (deleteNext) {
					deleteNext = false;
					p = result[path].erase(p);
					p--;
					continue;
				}
				if (p->z < 0) {
					auto shift = -p->z;
					if (shift & 0b10000) {
						bool deleted = false;
						while (std::abs(prev->x - p->x) + std::abs(prev->y - p->y) <= 2) {
							deleted = true;
							p = result[path].erase(prev);
							if (p != result[path].begin()) {
								--index;
								prev = p - 1;
							}
							else {
								prev = result[path].end() - 1;
							}
						}
						if (!deleted)
							deleteNext = true;
					}
					if (shift & 0b1000)
						p->x -= 1;
					if (shift & 0b0100)
						p->x += 1;
					if (shift & 0b0010)
						p->y += 1;
					if (shift & 0b0001)
						p->y -= 1;

					bool testSucceded = false;
					for (auto& pat : itemsWithNoFit[i]->innerFit) {
						if (Clipper2Lib::PointInPolygon(*p, pat) != Clipper2Lib::PointInPolygonResult::IsOutside) {
							testSucceded = true;
							break;
						}
					}
					if (!testSucceded) {
						std::cout << p->x << "," << p->y << "\n\n";
						for (auto& point : itemsWithNoFit[i]->innerFit[0]) {
							if (*p == point)
								std::cout << "   ";
							std::cout << point.x << "," << point.y << "\n";
						}
						std::cout << "\n";
						for (auto& point : noFit[0]) {
							std::cout << point.x << "," << point.y << "\n";
						}
						std::cerr << "new Point inside old IFP\n";
					}
				}
				if (p->x == prev->x && p->y == prev->y) {
					p = result[path].erase(p);
					p--;
					continue;
				}
				prev = p;
				p->z = index;
				++index;
			}
			if (deleteNext) {
				result[path].erase(result[path].begin());
			}
		}*/
		itemsWithNoFit[i]->innerFit = result;

	}
}

void IncrementalNoFitSolver::addNewPiece(ItemWithNoFit* item, Point64& translation) {
	/*translation.x = (translation.x / scaleFactor * scaleFactor);
	translation.y = (translation.y / scaleFactor * scaleFactor);
	Transformation translate(CGAL::TRANSLATION, Vector(NT(((double)translation.x / (double)scaleFactor)), NT(((double)translation.y / (double)scaleFactor))));
	Candidate addedCand({item->item->index, transform(translate, item->item->poly), (long)(translation.x / scaleFactor), (long)(translation.y / scaleFactor)});
	if(!reorderItems)
		problem->addCandidate(addedCand, item->item->value);*/
	if (!reorderItems)
		problem->addCandidate(item->item, translation.x / scaleFactor, translation.y / scaleFactor);
	updateNoFits(item, translation);
	additionalUpdates(item, translation);
	//placedPieces = Clipper2Lib::Union(placedPieces, { newPiece }, Clipper2Lib::FillRule::NonZero);
}


bool IncrementalNoFitSolver::findBestItem(ItemWithNoFit*& bestItem, Point64& translation) {
	bool itemFound = false;

	int64_t bestEval = compareEval(1, 0) == ComparisonResult::BETTER ? std::numeric_limits<int64_t>::min() : std::numeric_limits<int64_t>::max();
	for (auto it = itemsWithNoFit.begin(); it!=itemsWithNoFit.end(); ++it) {
		Point64 attachmentPoint;
		int64_t eval = 0;
		if ((*it)->quantity > 0 && findBestPlacement(*it, attachmentPoint, eval)) {

			if (bestFit) {
				if (!itemFound || ItemIsBetter(*it, attachmentPoint, eval, bestItem, translation, bestEval)){
					itemFound = true;
					bestEval = eval;
					bestItem = (*it);
					translation = attachmentPoint;
				}
			}
			else {
				itemFound = true;
				bestItem = (*it);
				translation = attachmentPoint;
				break;
			}
		}
		else {
			(*it)->quantity = 0;
		}
	}
	return itemFound;
}

bool IncrementalNoFitSolver::findBestPlacement(ItemWithNoFit* testedItem, Point64& attachmentPoint, int64_t& eval) {
	//find best position for given item inside current container

	bool positionFound = false;
	eval = 0;

	switch (placementMode)
	{
	case PlacementStrategy::BOTTOM_LEFT: {
		attachmentPoint = Point64(std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max());
		for (auto& area : testedItem->innerFit) {
			for (auto& p : area) {
				if (p.y < attachmentPoint.y || (p.y == attachmentPoint.y && p.x < attachmentPoint.x)) {
					attachmentPoint = p;
					positionFound = true;
				}
			}
		}
		break;
	}
	case PlacementStrategy::BOTTOM_RIGHT: {
		attachmentPoint = Point64(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max());
		for (auto& area : testedItem->innerFit) {
			for (auto& p : area) {
				if (p.y < attachmentPoint.y || (p.y == attachmentPoint.y && p.x > attachmentPoint.x)) {
					attachmentPoint = p;
					positionFound = true;
				}
			}
		}
		break;
	}
	case PlacementStrategy::TOP_LEFT: {
		attachmentPoint = Point64(std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::min());
		for (auto& area : testedItem->innerFit) {
			for (auto& p : area) {
				if (p.y > attachmentPoint.y || (p.y == attachmentPoint.y && p.x < attachmentPoint.x)) {
					attachmentPoint = p;
					positionFound = true;
				}
			}
		}
		break;
	}
	case PlacementStrategy::TOP_RIGHT: {
		attachmentPoint = Point64(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min());
		for (auto& area : testedItem->innerFit) {
			for (auto& p : area) {
				if (p.y > attachmentPoint.y || (p.y == attachmentPoint.y && p.x > attachmentPoint.x)) {
					attachmentPoint = p;
					positionFound = true;
				}
			}
		}
		break;
	}
	case PlacementStrategy::MIN_DIST: {
		int64_t minDist = std::numeric_limits<int64_t>::max();
		for (auto& area : testedItem->innerFit) {
			for (auto& p : area) {
				if (approxDistance(p.x, p.y) < minDist) {//(p.y < attachmentPoint.y || (p.y == attachmentPoint.y && p.x < attachmentPoint.x)) {
					attachmentPoint = p;
					minDist = approxDistance(p.x, p.y);
					positionFound = true;
				}
			}
		}
		eval = minDist;
		break;
	}
	case PlacementStrategy::CONCAVE_FIT: {
		double bestMatch = -1;
		for (auto& area : testedItem->innerFit) {
			for (size_t i = 0; i < area.size(); i++) {
				Point64 previousEdge = i > 0 ? area[i - 1] - area[i] : area[area.size() - 1] - area[i];
				Point64 nextEdge = i < area.size() - 1 ? area[i + 1] - area[i] : area[0] - area[i];
				double dot = ((double)(previousEdge.x * nextEdge.x + previousEdge.y * nextEdge.y)) / ((double)(approxDistance(previousEdge.x, previousEdge.y) * approxDistance(nextEdge.x, nextEdge.y)));
				if (dot > bestMatch) {
					positionFound = true;
					attachmentPoint = area[i];
					bestMatch = dot;
				}
			}
		}
		break;
	}
	case PlacementStrategy::TOPOS: {
		eval = std::numeric_limits<int64_t>::max();
		for (auto& area : testedItem->innerFit) {
			for (auto& p : area) {
				int64_t currentEval = 0;
				BoundingBox testedBBox = { testedItem->bottomLeft + p, testedItem->topRight + p, (testedItem->bottomLeft + testedItem->topRight) * 0.5 + p };
				Point64 newBL = { std::min(BBoxPlaced.bottomLeft.x, testedBBox.bottomLeft.x), std::min(BBoxPlaced.bottomLeft.y, testedBBox.bottomLeft.y) };
				Point64 newTR = { std::max(BBoxPlaced.topRight.x, testedBBox.topRight.x), std::max(BBoxPlaced.topRight.y, testedBBox.topRight.y) };
				//waste
				currentEval += (newTR.x - newBL.x) * (newTR.y - newBL.y) - (BBoxPlaced.topRight.x - BBoxPlaced.bottomLeft.x) * (BBoxPlaced.topRight.y - BBoxPlaced.bottomLeft.y);

				//overlap
				int64_t overlap = 0;
				for (auto& bbox : placedBBoxes) {
					if (testedBBox.bottomLeft.x < bbox.topRight.x && testedBBox.bottomLeft.y < bbox.topRight.y) {
						if (testedBBox.topRight.x > bbox.bottomLeft.x && testedBBox.topRight.y > bbox.bottomLeft.y) {
							Point64 bl = { std::max(testedBBox.bottomLeft.x, bbox.bottomLeft.x), std::max(testedBBox.bottomLeft.y, bbox.bottomLeft.y) };
							Point64 tr = { std::min(testedBBox.topRight.x, bbox.topRight.x), std::min(testedBBox.topRight.y, bbox.topRight.y) };
							overlap += (tr.x - bl.x) * (tr.y - bl.y);
						}
					}
				}
				currentEval -= overlap;

				//distance
				currentEval += approxDistance(testedBBox.center.x - BBoxPlaced.center.x, testedBBox.center.y - BBoxPlaced.center.y);

				if (currentEval < eval) {
					positionFound = true;
					eval = currentEval;
					attachmentPoint = p;
				}

			}
		}
		break;
	}
	default:
		break;
	}

	return positionFound;
}

bool IncrementalNoFitSolver::ItemIsBetter(ItemWithNoFit* item1, Point64& translation1, int64_t eval1, ItemWithNoFit* item2, Point64& translation2, int64_t eval2) {

	switch (placementMode)
	{
	case PlacementStrategy::BOTTOM_LEFT:
		if (translation1.y < translation2.y)
			return true;
		else if (translation1.y == translation2.y && translation1.x < translation2.x)
			return true;
		else
			return false;
		break;
	case PlacementStrategy::BOTTOM_RIGHT:
		break;
	case PlacementStrategy::TOP_LEFT:
		break;
	case PlacementStrategy::TOP_RIGHT:
		break;
	case PlacementStrategy::MIN_DIST:
		return eval1 < eval2;
		break;
	case PlacementStrategy::CONCAVE_FIT:
		break;
	case PlacementStrategy::TOPOS:
		return eval1 / item1->item->value < eval2 / item2->item->value;
		break;
	default:
		break;
	}
	

	return false;
}


void IncrementalNoFitSolver::getSolutionOrder(Problem* problem, std::vector<size_t>& order) {
	solve(problem);
	order = bestFitOrder;
	problem->resetCandidates();
}

void IncrementalNoFitSolver::getBestFitOrder(Problem* problem, std::vector<size_t>& order) {
	IncrementalNoFitSolver solver = IncrementalNoFitSolver();
	solver.bestFit = true;
	solver.reorderItems = true;
	solver.getSolutionOrder(problem, order);
}