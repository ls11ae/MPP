#include "innerfitraster.h"

int64_t InnerFitRaster::xRes = 1000;
int64_t InnerFitRaster::yRes = 1000;

SolveStatus InnerFitRaster::solve(Problem* prob) {
	items = prob->getItems();
	std::vector<size_t> ord = {};
	for (size_t i = 0; i < items.size(); i++)
	{
		for (size_t j = 0; j < items[i]->quantity; j++) {
			ord.push_back(i);
		}
	}
	return solve(prob, ord);
}
SolveStatus InnerFitRaster::solve(Problem* prob, std::vector<size_t>& order)
{
	problem = prob;
	this->order = order;
	nextIndex = this->order.begin();
	int c = 0;

	initIFRs();
	while (true)
	{
		ItemWithIFR* bestItem = NULL;
		Point64 translation;

		if (!findNextItem(bestItem, translation)) {
			break;
		}
		else {
			--bestItem->quantity;
			if (bestItem->quantity < 0) {
				return SolveStatus::Unsolved;
			}
		}
		addNewPiece(bestItem, translation);
		++c;
		if (VERBOSE && (c % 100 == 0)) {
			std::cout << problem->getString() << ": " << c << " pieces placed\n";
		}
		//toIPE2("test.ipe", problem->getContainer(), { placedPieces }, {});
	}
	switch (placementMode)
	{
	case PlacementRule::BOTTOM_LEFT:
		prob->setPlacementStrategy("BL");
		break;
	case PlacementRule::LEFT_BOTTOM:
		prob->setPlacementStrategy("LB");
		break;
	case PlacementRule::VZZ:
		prob->setPlacementStrategy("VZZ");
		break;
	case PlacementRule::HZZ:
		prob->setPlacementStrategy("HZZ");
		break;
	case PlacementRule::SPIRAL:
		prob->setPlacementStrategy("SPIRAL");
		break;
	case PlacementRule::ANTI_SPIRAL:
		prob->setPlacementStrategy("ASPIRAL");
		break;
	default:
		break;
	}

	for (size_t i = 0; i < itemsWithIFR.size(); i++)
	{
		delete itemsWithIFR[i];
	}
	return SolveStatus::Feasible;
}


void InnerFitRaster::initIFRs() {

	if (!reset) {
		//Initialize no-fit polygon for each item as polygon around container with a hole for each free space

		container = problem->getContainer();


		int wall = 10e6;

		xStepSize = (container.right_vertex()->x() - container.left_vertex()->x()).interval().sup() / xResolution;
		yStepSize = (container.top_vertex()->y() - container.bottom_vertex()->y()).interval().sup() / yResolution;

		//build new polygon as outside of container

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
	
	itemsWithIFR = std::vector<ItemWithIFR*>(items.size(), nullptr);
	#pragma omp parallel for
	for (int i = 0; i < items.size(); i++) {

		//initialize inner-fit raster
		auto ifr = IFR();

		Polygon inverse;
		for (size_t j = 0; j < items[i]->poly.vertices().size(); j++)
		{
			inverse.push_back({ -items[i]->poly[j].x(), -items[i]->poly[j].y() });
		}

		auto innerFitHoles = CGAL::minkowski_sum_2(container, inverse).holes();
		Paths innerFit;

		for (auto& innerFitHole : innerFitHoles) {
			innerFitHole.reverse_orientation();
			Path hole;
			for (auto p : innerFitHole)
			{
				hole.push_back({ CGAL::to_double(p.x()), CGAL::to_double(p.y()) , 999 });
			}
			innerFit.push_back(Clipper2Lib::TrimCollinear(hole));
		}

		if (innerFit.size() == 0) {
			ifr.raster = { {false} };
		}
		else {
			Point64 shift;
			polygonToRaster(innerFit[0], xStepSize, yStepSize, ifr.raster, ifr.shift);
		}

		if (reset) {
			//only reset IFR and 
			itemsWithIFR[i]->ifr = ifr;
			itemsWithIFR[i]->quantity = items[i]->quantity;
		}
		else {
			//initialize decomposition and bounding box
			std::vector<Point> points = items[i]->poly.vertices();
			std::list<Partition_traits::Polygon_2> polyDecompIndices;

			convexDecompositionIndices(points, polyDecompIndices);

			std::vector<ConvexEdgeList> polyDecomp;
			std::vector<ConvexEdgeList> inverseDecomp;

			edgeListsFromDecomposition(points, polyDecompIndices, polyDecomp, inverseDecomp);



			//find top-right of bounding box
			int64_t maxX = std::numeric_limits<int64_t>::min();
			int64_t maxY = std::numeric_limits<int64_t>::min();

			for (auto& p : items[i]->poly) {
				int64_t currentX = CGAL::to_double(p.x());
				int64_t currentY = CGAL::to_double(p.y());
				if (currentX > maxX)
					maxX = currentX;
				if (currentY > maxY)
					maxY = currentY;
			}

			itemsWithIFR[i] = new ItemWithIFR({ items[i], items[i]->quantity, ifr, polyDecomp, inverseDecomp, maxX, maxY });
		}
	}
	reset = true;
}

void InnerFitRaster::updateNoFits(ItemWithIFR* addedPiece, Point64& translation) {

	//update no-fit polygons

	//long t1 = 0, t2 = 0;
	int64_t minkowskiTime = 0;
	int64_t sampleTime = 0;
	//std::for_each(std::execution::seq, std::begin(itemsWithIFR), std::end(itemsWithIFR), [&](ItemWithIFR* it)
	#pragma omp parallel for
	for (int i = 0; i < itemsWithIFR.size(); i++)
	{
		if (itemsWithIFR[i]->quantity <= 0)
			continue;
		//auto start = std::chrono::high_resolution_clock::now();
		Paths noFitParts = minkowskiSum(addedPiece->convexDecomp, itemsWithIFR[i]->inversePoly, {translation.x * xStepSize, translation.y * yStepSize});
			
		int64_t xMax = std::min((addedPiece->xSpan + itemsWithIFR[i]->xSpan) / xStepSize + 1, xResolution - translation.x);
		int64_t yMax = std::min((addedPiece->ySpan + itemsWithIFR[i]->ySpan) / yStepSize + 1, yResolution - translation.y);
		auto noFit = Clipper2Lib::Union(noFitParts, Clipper2Lib::FillRule::NonZero)[0];
		//auto end = std::chrono::high_resolution_clock::now();
		//minkowskiTime += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
		/*for (int x = resolution - 1; x >= 0; x--)
		{
			for (int y = 0; y < resolution; y++)
			{
				std::cout << it->innerFitRaster[x][y] << " ";
			}
			std::cout << "\n";
		}*/
		//start = std::chrono::high_resolution_clock::now();
		std::vector<std::vector<bool>> noFitRaster;
		Point64 shift;
		polygonToRaster(noFit, xStepSize, yStepSize, noFitRaster, shift);
		for (int x = std::max(0, (int) (itemsWithIFR[i]->ifr.shift.x - shift.x)); x < noFitRaster.size() && x < ((int) itemsWithIFR[i]->ifr.raster.size()) - shift.x + itemsWithIFR[i]->ifr.shift.x; x++)
		{
			for (int y = std::max(0, (int)(itemsWithIFR[i]->ifr.shift.y - shift.y)); y < noFitRaster[x].size() && y < ((int) itemsWithIFR[i]->ifr.raster[0].size()) - shift.y + itemsWithIFR[i]->ifr.shift.y; y++)
			{
				if (noFitRaster[x][y]) {
					itemsWithIFR[i]->ifr.raster[x + shift.x - itemsWithIFR[i]->ifr.shift.x][y + shift.y - itemsWithIFR[i]->ifr.shift.y] = false;
				}
			}
		}
		//end = std::chrono::high_resolution_clock::now();
		//sampleTime += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
		/*
		std::cout << "\n";
		for (int x = resolution-1; x >= 0; x--)
		{
			for (int y = 0; y < resolution; y++)
			{
				std::cout << it->innerFitRaster[x][y] << " ";
			}
			std::cout << "\n";
		}*/
	}
	//std::cout << "Minkowski " << minkowskiTime/1000 << "ms, Sample " << sampleTime/1000 << "ms\n";
}

void InnerFitRaster::addNewPiece(ItemWithIFR* item, Point64& translation) {
	/*Transformation translate(CGAL::TRANSLATION, Vector(NT((long)(translation.x * xStepSize)), NT((long)(translation.y * yStepSize))));
	Candidate addedCand({ item->item->index, transform(translate, item->item->poly), (long)(translation.x * xStepSize), (long)(translation.y * yStepSize) });
	problem->addCandidate(addedCand, item->item->value);*/
	problem->addCandidate(item->item, (translation.x * xStepSize), (translation.y * yStepSize));
	updateNoFits(item, translation);
}


bool InnerFitRaster::findNextItem(ItemWithIFR*& bestItem, Point64& translation) {
	bool itemFound = false;

	for (; nextIndex != order.end(); ++nextIndex)
	{
		Point64 attachmentPoint;
		if (findBestPlacement(itemsWithIFR[*nextIndex], attachmentPoint)) {
			itemFound = true;
			bestItem = (itemsWithIFR[*nextIndex]);
			translation = attachmentPoint;
			++nextIndex;
			break;
		}
	}
	return itemFound;
}

bool InnerFitRaster::findBestPlacement(ItemWithIFR* testedItem, Point64& attachmentPoint) {
	//find best position for given item inside current container
	
	size_t xSize = testedItem->ifr.raster.size();
	size_t ySize = testedItem->ifr.raster[0].size();

	switch (placementMode)
	{
	case PlacementRule::BOTTOM_LEFT: {
		for (size_t x = 0; x < testedItem->ifr.raster.size(); x++) {
			for (size_t y = 0; y < testedItem->ifr.raster[0].size(); y++) {
				if (testedItem->ifr.raster[x][y]) {
					attachmentPoint = Point64({ x,y }) + testedItem->ifr.shift;
					return true;
				}
			}
		}
		break;
	}
	case PlacementRule::LEFT_BOTTOM: {
		for (size_t y = 0; y < testedItem->ifr.raster[0].size(); y++) {
			for (size_t x = 0; x < testedItem->ifr.raster.size(); x++) {
				if (testedItem->ifr.raster[x][y]) {
					attachmentPoint = Point64({ x,y }) + testedItem->ifr.shift;
					return true;
				}
			}
		}
		break;
	}
	case PlacementRule::VZZ: {
		for (size_t x = 0; x < testedItem->ifr.raster.size(); x++) {
			if (x % 2 == 0) {
				for (size_t y = 0; y < testedItem->ifr.raster[0].size(); y++) {
					if (testedItem->ifr.raster[x][y]) {
						attachmentPoint = Point64({ x,y }) + testedItem->ifr.shift;
						return true;
					}
				}
			}
			else {
				for (size_t y = testedItem->ifr.raster[0].size()-1; y < testedItem->ifr.raster[0].size(); y--) {
					if (testedItem->ifr.raster[x][y]) {
						attachmentPoint = Point64({ x,y }) + testedItem->ifr.shift;
						return true;
					}
				}
			}
		}
		break;
	}
	case PlacementRule::HZZ: {
		for (size_t y = 0; y < testedItem->ifr.raster[0].size(); y++) {
			if (y % 2 == 0) {
				for (size_t x = 0; x < testedItem->ifr.raster.size(); x++) {
					if (testedItem->ifr.raster[x][y]) {
						attachmentPoint = Point64({ x,y }) + testedItem->ifr.shift;
						return true;
					}
				}
			}
			else {
				for (size_t x = testedItem->ifr.raster.size() - 1; x < testedItem->ifr.raster.size(); x--) {
					if (testedItem->ifr.raster[x][y]) {
						attachmentPoint = Point64({ x,y }) + testedItem->ifr.shift;
						return true;
					}
				}
			}
		}
		break;
	}
	case PlacementRule::SPIRAL: {
		size_t x = 0;
		size_t y = 0;
		for (size_t k = 0; k < 2*std::min(xSize, ySize); k++)
		{
			if (k % 4 == 0) {
				for (size_t m = 0; m < ySize - 2 * (k / 4); m++) {
					if (testedItem->ifr.raster[x][y]) {
						attachmentPoint = Point64({ x,y }) + testedItem->ifr.shift;
						return true;
					}
					y += 1;
				}
				x += 1;
				y -= 1;
			}
			else if (k % 4 == 1) {
				for (size_t n = 1; n < xSize - 2 * (k / 4); n++) {
					if (testedItem->ifr.raster[x][y]) {
						attachmentPoint = Point64({ x,y }) + testedItem->ifr.shift;
						return true;
					}
					x += 1;
				}
				x -= 1;
				y -= 1;
			}
			else if (k % 4 == 2) {
				for (size_t m = 1; m < ySize - 2 * (k / 4); m++) {
					if (testedItem->ifr.raster[x][y]) {
						attachmentPoint = Point64({ x,y }) + testedItem->ifr.shift;
						return true;
					}
					y -= 1;
				}
				x -= 1;
				y += 1;
			}
			else if (k % 4 == 3) {
				for (size_t n = 2; n < xSize - 2 * (k / 4); n++) {
					if (testedItem->ifr.raster[x][y]) {
						attachmentPoint = Point64({ x,y }) + testedItem->ifr.shift;
						return true;
					}
					x -= 1;
				}
				x += 1;
				y += 1;
			}
		}
		break;
	}
	case PlacementRule::ANTI_SPIRAL: {
		size_t x = 0;
		size_t y = 0;
		for (size_t k = 0; k < 2 * std::min(xSize, ySize); k++)
		{
			if (k % 4 == 0) {
				for (size_t n = 0; n < xSize - 2 * (k / 4); n++) {
					if (testedItem->ifr.raster[x][y]) {
						attachmentPoint = Point64({ x,y }) + testedItem->ifr.shift;
						return true;
					}
					x += 1;
				}
				x -= 1;
				y += 1;
			}
			else if (k % 4 == 1) {
				for (size_t m = 1; m < ySize - 2 * (k / 4); m++) {
					if (testedItem->ifr.raster[x][y]) {
						attachmentPoint = Point64({ x,y }) + testedItem->ifr.shift;
						return true;
					}
					y += 1;
				}
				x -= 1;
				y -= 1;
			}
			else if (k % 4 == 2) {
				for (size_t n = 1; n < xSize - 2 * (k / 4); n++) {
					if (testedItem->ifr.raster[x][y]) {
						attachmentPoint = Point64({ x,y }) + testedItem->ifr.shift;
						return true;
					}
					x -= 1;
				}
				x += 1;
				y -= 1;
			}
			else if (k % 4 == 3) {
				for (size_t m = 2; m < ySize - 2 * (k / 4); m++) {
					if (testedItem->ifr.raster[x][y]) {
						attachmentPoint = Point64({ x,y }) + testedItem->ifr.shift;
						return true;
					}
					y -= 1;
				}
				x += 1;
				y += 1;
			}
		}
		break;
	}
	default:
		break;
	}

	return false;
}