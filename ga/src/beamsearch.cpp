#include "beamsearch.h"

SolveStatus BeamSearch::solve(Problem* prob)
{
	problem = prob;
	int c = 0;

	initIFs();
	additionalInits();

	std::vector<std::vector<ItemPlacement>> beamPlacements = { {} };
	size_t currentWidth = 1;
	size_t depth = 0;
	int64_t bestFinalScore = -1;
	std::vector<ItemPlacement> bestFinalBeam = {};
	while (true)
	{
		std::list<FeasiblePlacement> nextLayer;
		//find each beam's best children by local eval
		for (size_t beam = 0; beam < currentWidth; beam++)
		{
			bool itemFound = false;
			std::list<FeasiblePlacement> beamChildren;
			size_t currentFilter = filterWidth;
			if (currentWidth == 1)
				currentFilter = beamWidth;
			for (auto& nextItem : beamItems) {
				Point64 translation;
				std::vector<int64_t> eval;
				if (nextItem->quantities[beam] <= 0 || !findBestPlacement(nextItem, beam, translation, eval))
					continue;
				
				itemFound = true;
				if (beamChildren.size() < currentFilter || evalIsBetter(eval, beamChildren.back().eval)) {
					FeasiblePlacement newPlacement = { eval, nextItem, translation, beam };
					auto it = beamChildren.begin();
					for (; it != beamChildren.end() && evalIsBetter(it->eval, eval); ++it);
					beamChildren.insert(it, newPlacement);
				}
				if (beamChildren.size() > currentFilter)
					beamChildren.pop_back();
			}
			if (!itemFound) {
				int64_t beamScore = 0;
				for (auto& seq : beamPlacements[beam])
				{
					beamScore += seq.item->value;
				}
				if (VERBOSE)
					std::cout << "Beam " << beam << " returned score " << beamScore << "\n";
				if (beamScore > bestFinalScore) {
					bestFinalScore = beamScore;
					bestFinalBeam = beamPlacements[beam];
				}
			}
			else {
				nextLayer.splice(nextLayer.end(), beamChildren);
			}
		}

		//global eval
		if (nextLayer.size() > beamWidth) {
			nextLayer.sort([&](FeasiblePlacement& a, FeasiblePlacement& b) {
				double valArea1 = (packedValues[a.parentBeam] + a.item->item->value) / combinedArea(BBoxPlaced[a.parentBeam], a);
				double valArea2 = (packedValues[b.parentBeam] + b.item->item->value) / combinedArea(BBoxPlaced[b.parentBeam], b);
				return valArea1 > valArea2;
				});

			auto cutoff = nextLayer.begin();
			std::advance(cutoff, beamWidth);
			nextLayer.erase(cutoff, nextLayer.end());
		}

		currentWidth = nextLayer.size();
		if (nextLayer.empty())
			break;
		std::vector<std::vector<Paths>> innerFits(beamItems.size(), std::vector<Paths>());
		std::vector<std::vector<int>> quantities(beamItems.size(), std::vector<int>());
		std::vector<std::vector<ItemPlacement>> beamSequences(currentWidth, std::vector<ItemPlacement>({}));
		std::vector<std::list<BoundingBox>> beamPlacedBBoxes(currentWidth);
		std::vector<BoundingBox> beamBBoxes = std::vector<BoundingBox>();
		std::vector<double> beamOccupied = std::vector<double>();
		std::vector<double> beamValues = std::vector<double>();
		beamBBoxes.reserve(currentWidth);
		beamOccupied.reserve(currentWidth);
		beamValues.reserve(currentWidth);

		//calculate updated version for each inner fit
		#pragma omp parallel for
		for (int i=0; i<beamItems.size(); i++)
		{
			for (auto placement = nextLayer.begin(); placement != nextLayer.end(); ++placement)
			{
				Paths newIF;
				updatedInnerFit(beamItems[i], *placement, newIF);
				innerFits[i].push_back(newIF);
				if (beamItems[i] == placement->item)
					quantities[i].push_back(beamItems[i]->quantities[placement->parentBeam] - 1);
				else
					quantities[i].push_back(beamItems[i]->quantities[placement->parentBeam]);
			}
		}

		//write updates to stored beams
		for (size_t i = 0; i < beamItems.size(); i++)
		{
			beamItems[i]->innerFits = innerFits[i];
			beamItems[i]->quantities = quantities[i];
		}

		//update beam sequences and bounding boxes
		size_t index = 0;
		for (auto placement = nextLayer.begin(); placement != nextLayer.end(); ++placement)
		{
			beamSequences[index] = beamPlacements[placement->parentBeam];
			beamSequences[index].push_back(ItemPlacement({ placement->item->item, placement->translation }));

			std::list<BoundingBox> individuals;
			BoundingBox total;
			updatedBBoxes(*placement, individuals, total);
			beamBBoxes.push_back(total);
			beamPlacedBBoxes[index] = individuals;
			beamValues.push_back(packedValues[placement->parentBeam] + placement->item->item->value);

			index++;
		}

		BBoxPlaced = beamBBoxes;
		placedBBoxes = beamPlacedBBoxes;
		beamPlacements = beamSequences;
		packedValues = beamValues;
		depth++;
		/*for (size_t i = 0; i < beamPlacements.size(); i++)
		{
			std::cout << depth << "\n";
			for (size_t j = 0; j < beamPlacements[i].size(); j++) {
				std::cout << beamPlacements[i][j].item->index << ", ";
			}
			std::cout << "\n";
		}*/
	}
	//toIPE2("test.ipe", problem->getContainer(), { placedPieces }, {});

	//apply best scoring beam
	for (auto &it: bestFinalBeam)
	{
		prob->addCandidate(it.item, (it.translation.x / scaleFactor), (it.translation.y / scaleFactor));
	}
	addSpecifics();


	while (!beamItems.empty()) delete beamItems.back(), beamItems.pop_back();
	return SolveStatus::Feasible;
}

void BeamSearch::addSpecifics() {
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

void BeamSearch::initIFs() {
	//Initialize no-fit polygon for each item as polygon around container with a hole for each free space

	container = problem->getContainer();

	for (auto& v : container) {
		pathContainer.push_back({ v.x().interval().sup() * scaleFactor, v.y().interval().sup() * scaleFactor });
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


	Polygon placedPoly;
	for (auto& placedPart : placedPieces) {
		for (auto& p : placedPart) {
			placedPoly.push_back({ (p.x / scaleFactor), (p.y / scaleFactor) });
		}
	}

	for (size_t i = 0; i < items.size(); i++) {
		Polygon inverse;
		Partition_traits::Polygon_2 polyIndices;
		for (size_t j = 0; j < items[i]->poly.vertices().size(); j++)
		{
			inverse.push_back({ -items[i]->poly[j].x(), -items[i]->poly[j].y() });
			polyIndices.push_back(j);
		}
		
		//calculate decomposition
		std::vector<Point> points = items[i]->poly.vertices();
		std::list<Partition_traits::Polygon_2> polyDecompIndices;

		convexDecompositionIndices(points, polyDecompIndices);

		std::vector<ConvexEdgeList> polyDecomp;
		std::vector<ConvexEdgeList> inverseDecomp;

		edgeListsFromDecomposition(points, polyDecompIndices, polyDecomp, inverseDecomp);

		//pathsToIPE("test.ipe", container, { inverseNext });
		auto innerFitHoles = CGAL::minkowski_sum_2(container, inverse).holes();
		Paths innerFit;

		for (auto innerFitHole : innerFitHoles) {
			innerFitHole.reverse_orientation();
			Path hole;
			for (auto p : innerFitHole)
			{
				Point64 clipperP(p.x().interval().sup(), p.y().interval().sup(), 1);
				hole.push_back({ CGAL::to_double(p.x()) * scaleFactor, CGAL::to_double(p.y()) * scaleFactor, 999 });
			}
			innerFit.push_back(Clipper2Lib::TrimCollinear(hole));
		}
		innerFit = Clipper2Lib::InflatePaths(innerFit, -12, Clipper2Lib::JoinType::Square, Clipper2Lib::EndType::Polygon);


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

		beamItems.push_back(new BeamItem({ items[i], {items[i]->quantity}, {innerFit}, polyDecomp, inverseDecomp, bl, tr }));

		//init empty BBoxes
		Point64 bottomLeftPlaced = Point64(std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max());
		Point64 topRightPlaced = Point64(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min());
		BBoxPlaced = { { bottomLeftPlaced, topRightPlaced, {0,0} } };
		placedBBoxes = { {} };
		packedValues = { 0.0 };
	}
}

void BeamSearch::updatedBBoxes(FeasiblePlacement& placement, std::list<BoundingBox>& individualBBoxes, BoundingBox& totalBBox) {

	BoundingBox addedBBox = { placement.item->bottomLeft + placement.translation, placement.item->topRight + placement.translation,
				placement.translation + (placement.item->topRight + placement.item->bottomLeft) * 0.5 };

	//add new bbox
	std::copy(placedBBoxes[placement.parentBeam].begin(), placedBBoxes[placement.parentBeam].end(), std::back_inserter(individualBBoxes));
	individualBBoxes.push_back(addedBBox);
	if (individualBBoxes.size() > 50)
		individualBBoxes.pop_front();

	//update placed bbox
	totalBBox = BBoxPlaced[placement.parentBeam];
	if (addedBBox.bottomLeft.x < totalBBox.bottomLeft.x)
		totalBBox.bottomLeft.x = addedBBox.bottomLeft.x;
	if (addedBBox.bottomLeft.y < totalBBox.bottomLeft.y)
		totalBBox.bottomLeft.y = addedBBox.bottomLeft.y;
	if (addedBBox.topRight.x > totalBBox.topRight.x)
		totalBBox.topRight.x = addedBBox.topRight.x;
	if (addedBBox.topRight.y > totalBBox.topRight.y)
		totalBBox.topRight.y = addedBBox.topRight.y;
	totalBBox.center = totalBBox.bottomLeft * 0.5 + totalBBox.topRight * 0.5;

}

void BeamSearch::updatedInnerFit(BeamItem* item, FeasiblePlacement& addedPiece, Paths& result) {
	Paths noFitParts = minkowskiSum(addedPiece.item->convexDecomp, item->inversePoly, addedPiece.translation);

	auto noFit = Clipper2Lib::Union(noFitParts, Clipper2Lib::FillRule::NonZero);
	
	Clipper2Lib::Clipper64 c;
	auto subject = Clipper2Lib::InflatePaths(item->innerFits[addedPiece.parentBeam], -1, Clipper2Lib::JoinType::Miter, Clipper2Lib::EndType::Polygon);
	c.AddSubject(subject);
	c.AddClip(noFit);

	c.Execute(Clipper2Lib::ClipType::Difference, Clipper2Lib::FillRule::EvenOdd, result);
}


bool BeamSearch::findBestPlacement(BeamItem* testedItem, size_t beamID, Point64& attachmentPoint, std::vector<int64_t>& eval) {
	//find best position for given item inside current container

	bool positionFound = false;
	eval = { 0 };

	switch (placementMode)
	{
	case PlacementStrategy::BOTTOM_LEFT: {
		attachmentPoint = Point64(std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max());
		for (auto& area : testedItem->innerFits[beamID]) {
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
		for (auto& area : testedItem->innerFits[beamID]) {
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
		for (auto& area : testedItem->innerFits[beamID]) {
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
		for (auto& area : testedItem->innerFits[beamID]) {
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
		for (auto& area : testedItem->innerFits[beamID]) {
			for (auto& p : area) {
				if (approxDistance(p.x, p.y) < minDist) {//(p.y < attachmentPoint.y || (p.y == attachmentPoint.y && p.x < attachmentPoint.x)) {
					attachmentPoint = p;
					minDist = approxDistance(p.x, p.y);
					positionFound = true;
				}
			}
		}
		eval = { minDist };
		break;
	}
	case PlacementStrategy::CONCAVE_FIT: {
		double bestMatch = -1;
		for (auto& area : testedItem->innerFits[beamID]) {
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
		eval = std::vector<int64_t>(3, std::numeric_limits<int64_t>::max());
		for (auto& area : testedItem->innerFits[beamID]) {
			for (auto& p : area) {
				auto currentEval = std::vector<int64_t>(3, 0);
				BoundingBox testedBBox = { testedItem->bottomLeft + p, testedItem->topRight + p, (testedItem->bottomLeft + testedItem->topRight) * 0.5 + p };
				Point64 newBL = { std::min(BBoxPlaced[beamID].bottomLeft.x, testedBBox.bottomLeft.x), std::min(BBoxPlaced[beamID].bottomLeft.y, testedBBox.bottomLeft.y) };
				Point64 newTR = { std::max(BBoxPlaced[beamID].topRight.x, testedBBox.topRight.x), std::max(BBoxPlaced[beamID].topRight.y, testedBBox.topRight.y) };
				
				//waste
				currentEval[0] = (newTR.x - newBL.x) * (newTR.y - newBL.y) - (BBoxPlaced[beamID].topRight.x - BBoxPlaced[beamID].bottomLeft.x) * (BBoxPlaced[beamID].topRight.y - BBoxPlaced[beamID].bottomLeft.y);

				//overlap
				int64_t overlap = 0;
				for (auto& bbox : placedBBoxes[beamID]) {
					if (testedBBox.bottomLeft.x < bbox.topRight.x && testedBBox.bottomLeft.y < bbox.topRight.y) {
						if (testedBBox.topRight.x > bbox.bottomLeft.x && testedBBox.topRight.y > bbox.bottomLeft.y) {
							Point64 bl = { std::max(testedBBox.bottomLeft.x, bbox.bottomLeft.x), std::max(testedBBox.bottomLeft.y, bbox.bottomLeft.y) };
							Point64 tr = { std::min(testedBBox.topRight.x, bbox.topRight.x), std::min(testedBBox.topRight.y, bbox.topRight.y) };
							overlap += (tr.x - bl.x) * (tr.y - bl.y);
						}
					}
				}
				currentEval[1] = -overlap;

				//distance
				currentEval[2] = approxDistance(testedBBox.center.x - BBoxPlaced[beamID].center.x, testedBBox.center.y - BBoxPlaced[beamID].center.y);

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

bool BeamSearch::evalIsBetter(std::vector<int64_t>& e1, std::vector<int64_t>& e2) {
	switch (aggrMethod)
	{
	case ToposAggregation::SUM: {
		int64_t s1 = 0, s2 = 0;
		for (int i = 0; i < e1.size(); i++) {
			s1 += e1[i];
			s2 += e2[i];
		}
		return s1 < s2;
		break;
	}
	case ToposAggregation::VECTOR: {
		int firstWins = 0;
		for (int i = 0; i < e1.size(); i++) {
			if (e1[i] < e2[i])
				firstWins++;
			else if (e1[i] > e2[i])
				firstWins--;
		}
		return firstWins > 0;
		break;
	}
	case ToposAggregation::PRIORITY: {
		for (int i = 0; i < e1.size(); i++) {
			if (e1[i] < e2[i])
				return true;
			else if (e1[i] > e2[i])
				return false;
		}
		return false;
		break;
	}
	default:
		return false;
		break;
	}
}