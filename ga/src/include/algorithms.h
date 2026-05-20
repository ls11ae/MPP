#pragma once

#include "problem.hpp"

enum class ConvexDecompositionResult { OPTIMAL, APPROXIMATE };

typedef Path ConvexHull;

static Path minkowskiSum(const ConvexEdgeList& conv1, const ConvexEdgeList& conv2, const Point64& offset) {
	Point64 previous = conv1.start + conv2.start + offset;
	Path currentNoFit({ previous });

	size_t i = 0, j = 0;
	while (i < conv1.rightEdges.size() || j < conv2.rightEdges.size())
	{
		if (j >= conv2.rightEdges.size() || (i < conv1.rightEdges.size() && conv1.rightEdges[i].slope <= conv2.rightEdges[j].slope)) {
			previous = conv1.rightEdges[i].vec + previous;
			currentNoFit.push_back(previous);
			++i;
		}
		else {
			previous = conv2.rightEdges[j].vec + previous;
			currentNoFit.push_back(previous);
			++j;
		}
	}
	i = 0, j = 0;
	while (i < conv1.leftEdges.size() || j < conv2.leftEdges.size())
	{
		if (j >= conv2.leftEdges.size() || (i < conv1.leftEdges.size() && conv1.leftEdges[i].slope <= conv2.leftEdges[j].slope)) {
			previous = conv1.leftEdges[i].vec + previous;
			currentNoFit.push_back(previous);
			++i;
		}
		else {
			previous = conv2.leftEdges[j].vec + previous;
			currentNoFit.push_back(previous);
			++j;
		}
	}
	return Clipper2Lib::TrimCollinear(currentNoFit);
}

static Paths minkowskiSum(const std::vector<ConvexEdgeList>& p1, const std::vector<ConvexEdgeList>& p2, const Point64& offset) {
	Paths sum = Paths({});
	for (auto& p1Part : p1)
	{
		for (auto& p2Part : p2)
		{
			sum.push_back(minkowskiSum(p1Part, p2Part, offset));
		}
	}
	return sum;
}

static ConvexDecompositionResult convexDecompositionIndices(std::vector<Point>& points, std::list<Partition_traits::Polygon_2>& polyDecompIndices) {
	Partition_traits::Polygon_2 polyIndices;
	for (size_t j = 0; j < points.size(); j++)
	{
		polyIndices.push_back(j);
	}
	Partition_traits traits(CGAL::make_property_map(points));
	CGAL::optimal_convex_partition_2(polyIndices.vertices_begin(), polyIndices.vertices_end(), std::back_inserter(polyDecompIndices), traits);

	//check if decomposition failed, for some reason that happens sometimes
	if (!CGAL::convex_partition_is_valid_2(polyIndices.vertices_begin(),
		polyIndices.vertices_end(),
		polyDecompIndices.begin(),
		polyDecompIndices.end(),
		traits)) {
		
		polyDecompIndices = {};
		CGAL::approx_convex_partition_2(polyIndices.vertices_begin(), polyIndices.vertices_end(), std::back_inserter(polyDecompIndices), traits);
		assert(CGAL::convex_partition_is_valid_2(polyIndices.vertices_begin(),
			polyIndices.vertices_end(),
			polyDecompIndices.begin(),
			polyDecompIndices.end(),
			traits));
		return ConvexDecompositionResult::APPROXIMATE;
	}
	else
		return ConvexDecompositionResult::OPTIMAL;
}

static void edgeListsFromDecomposition(const std::vector<Point>& points, std::list<Partition_traits::Polygon_2> decompIndices, std::vector<ConvexEdgeList>& polyDecomp, std::vector<ConvexEdgeList>& inverseDecomp) {
	for (auto& partition : decompIndices) {
		auto previous = CGAL::Circulator_from_iterator(partition.begin(), partition.end(), partition.begin());
		auto start = previous;
		auto leftPoint = previous;
		//find leftmost point
		do {
			if (points[*previous] < points[*leftPoint])
				leftPoint = previous;
			++previous;
		} while (previous != start);

		previous = leftPoint;
		auto next = previous;
		++next;

		start = previous;
		bool rightHalf = true;
		Point64 startPoint(points[*previous].x().interval().sup(), points[*previous].y().interval().sup());
		std::vector<EdgeVector> rightEdges;
		std::vector<EdgeVector> leftEdges;

		Point64 inverseStartPoint;
		std::vector<EdgeVector> inverseRightEdges;
		std::vector<EdgeVector> inverseLeftEdges;
		do {
			int64_t x = (points[*next].x() - points[*previous].x()).interval().sup();
			int64_t y = (points[*next].y() - points[*previous].y()).interval().sup();
			EdgeVector ev(Point64(x, y));
			EdgeVector evInverse(Point64(-x, -y));

			if (rightHalf && x < 0) {
				rightHalf = false;
				inverseStartPoint = Point64(-points[*previous].x().interval().sup(), -points[*previous].y().interval().sup());
			}
			if (rightHalf) {
				rightEdges.push_back(ev);
				inverseLeftEdges.push_back(evInverse);
			}
			else {
				leftEdges.push_back(ev);
				inverseRightEdges.push_back(evInverse);
			}
			++previous;
			++next;
		} while (previous != start);
		polyDecomp.push_back({ startPoint, rightEdges, leftEdges });
		inverseDecomp.push_back({ inverseStartPoint, inverseRightEdges, inverseLeftEdges });
	}
}

static void updateConvexHull(ConvexHull& hull, const std::vector<Point64>& newPoints) {
	std::vector<Point> vertices = {};
	for (size_t i = 0; i < hull.size(); i++) {
		vertices.push_back(Point(hull[i].x, hull[i].y));
	}
	for (size_t i = 0; i < newPoints.size(); i++) {
		vertices.push_back(Point(newPoints[i].x, newPoints[i].y));
	}
	//calculate convex hull
	std::vector<std::size_t> indices(vertices.size()), CHIndices;
	std::iota(indices.begin(), indices.end(), 0);
	CGAL::convex_hull_2(indices.begin(), indices.end(), std::back_inserter(CHIndices),
		Convex_hull_traits(CGAL::make_property_map(vertices)));
	ConvexHull newHull = {};

	for (size_t i : CHIndices) {
		if (i < hull.size())
			newHull.push_back(hull[i]);
		else
			newHull.push_back(newPoints[i - hull.size()]);
	}
	hull = newHull;
}

enum class PointOnLineResult {
	LEFT,
	RIGHT,
	ON
};

static PointOnLineResult PointLineLocation(const Point64& segmentTop, const Point64& segmentBottom, const Point64& p) {
	auto det = (segmentBottom.x - segmentTop.x) * (p.y - segmentTop.y) - (segmentBottom.y - segmentTop.y) * (p.x - segmentTop.x);
	if (det < 0)
		return PointOnLineResult::LEFT;
	else if (det == 0)
		return PointOnLineResult::ON;
	else
		return PointOnLineResult::RIGHT;
}

static void polygonToRaster(const Path& poly, const int64_t xStep, const int64_t yStep, std::vector<std::vector<bool>>& raster, Point64& shift) {
	//line sweep algorithm to convert polygon to rasterized version
	auto bounds = Clipper2Lib::GetBounds(poly);
	raster = std::vector<std::vector<bool>>(bounds.Width() / xStep + 1, std::vector<bool>(bounds.Height() / yStep + 1, false));
	shift = { bounds.left / xStep, bounds.top / yStep };
	if (shift.x * xStep < bounds.left)
		shift.x++;
	if (shift.y * yStep < bounds.top)
		shift.y++;

	std::vector<size_t> indices(poly.size());
	std::iota(indices.begin(), indices.end(), 0);
	std::sort(indices.begin(), indices.end(),
		[&](size_t A, size_t B) -> bool {
			return poly[A].y < poly[B].y || (poly[A].y == poly[B].y && poly[A].x < poly[B].x);
		});
	//point indices sorted by descending y-value
	std::stack<size_t> events;
	for (auto i : indices) {
		events.push(i);
	}

	//top vertex  of currently limiting edges (alternating left and right)
	std::list<size_t> status = { events.top(), events.top() };
	events.pop();
	int64_t current_y = (poly[*(status.begin())].y / yStep) * yStep;
	if (current_y <= poly[*(status.begin())].y)
		current_y += yStep;
	while (!events.empty())
	{
		while (current_y - yStep > poly[events.top()].y || current_y - yStep == poly[events.top()].y && bounds.top == current_y - yStep) {
			current_y -= yStep;
			int64_t current_x = shift.x * xStep;
			bool leftBoundary = true;
			auto topVertex = status.begin();
			size_t bottomVertex;
			if (leftBoundary) {
				bottomVertex = *(topVertex)+1;
				if (bottomVertex >= poly.size())
					bottomVertex = 0;
			}
			else {
				if (*(topVertex) == 0)
					bottomVertex = poly.size() - 1;
				else
					bottomVertex = *(topVertex)-1;
			}
			int64_t nextIntersect = poly[*topVertex].x + ((current_y - poly[*topVertex].y) * (poly[bottomVertex].x - poly[*topVertex].x)) / (poly[bottomVertex].y - poly[*topVertex].y);
			while (true) {
				if (current_x > nextIntersect - xStep) {
					auto pointLocation = PointLineLocation(poly[*topVertex], poly[bottomVertex], Point64(current_x, current_y));

					++topVertex;
					if (topVertex != status.end()) {

						if (!leftBoundary) {
							bottomVertex = *(topVertex)+1;
							if (bottomVertex >= poly.size())
								bottomVertex = 0;
						}
						else {
							if (*(topVertex) == 0)
								bottomVertex = poly.size() - 1;
							else
								bottomVertex = *(topVertex)-1;
						}
						nextIntersect = poly[*topVertex].x + ((current_y - poly[*topVertex].y) * (poly[bottomVertex].x - poly[*topVertex].x)) / (poly[bottomVertex].y - poly[*topVertex].y);
					}

					if (pointLocation == PointOnLineResult::ON) {
						raster[current_x / xStep - shift.x][current_y / yStep - shift.y] = true;
					}
					else if (pointLocation == PointOnLineResult::LEFT) {
						raster[current_x / xStep - shift.x][current_y / yStep - shift.y] = !leftBoundary;
					}
					else {
						if (current_x > nextIntersect - xStep && topVertex != status.end())
							current_x -= xStep;
						else if (current_x / xStep - shift.x < raster.size()){
							
							raster[current_x / xStep - shift.x][current_y / yStep - shift.y] = leftBoundary;
						}
					}
					//std::cout << raster[current_x / xStep - shift.x][current_y / yStep - shift.y] << " ";
					if (topVertex == status.end())
						break;
					current_x += xStep;
					leftBoundary = !leftBoundary;
					continue;
				}
				if (!leftBoundary) {
					raster[current_x / xStep - shift.x][current_y / yStep - shift.y] = true;
				}
				current_x += xStep;
			}
		}
		bool leftBoundary = false;
		bool locationFound = false;

		for (auto it = status.begin(); it != status.end(); ++it) {
			leftBoundary = !leftBoundary;
			size_t segmentEnd;
			if (leftBoundary) {
				segmentEnd = *(it)+1;
				if (segmentEnd >= poly.size())
					segmentEnd = 0;
			}
			else {
				if (*(it) == 0)
					segmentEnd = poly.size() - 1;
				else
					segmentEnd = *(it)-1;
			}
			auto pointLocation = PointLineLocation(poly[*it], poly[segmentEnd], poly[events.top()]);
			if (pointLocation == PointOnLineResult::RIGHT)
				continue;
			else if (pointLocation == PointOnLineResult::LEFT) {
				if (locationFound)
					break;
				auto next = it;
				++next;

				//deal with self intersections due to imprecision
				if (next != status.end() && ((*next) == events.top() + 1 || (*next) == events.top() - 1) || (*next) == 0 && events.top() == poly.size() - 1 || events.top() == 0 && (*next) == poly.size() - 1) {
					continue;
				}
				status.insert(it, events.top());
				status.insert(it, events.top());
				locationFound = true;
				break;
			}
			else {
				if (locationFound) {
					--it;
					it = status.erase(it);
					it = status.erase(it);
					break;
				}
				it = status.erase(it);
				status.insert(it, events.top());
				locationFound = true;
				--it;
				continue;
			}
		}
		if (!locationFound) {
			status.push_back(events.top());
			status.push_back(events.top());
		}
		events.pop();
	}
}

enum class PolySweepEventType {
	SUBJECT_POINT,
	CLIP_POINT,
	INTERSECTION
};

enum class PolySweepEdgeType {
	NONE,
	SUBJECT_POINT_LEFT,
	SUBJECT_POINT_RIGHT,
	CLIP_POINT_LEFT,
	CLIP_POINT_RIGHT
};

struct PolygonPartReference {
	std::list<Point64>* poly;
	PolygonPartReference* leftSibling = NULL;
	PolygonPartReference* rightSibling = NULL;
	PolygonPartReference(std::list<Point64>* p) : poly(p) {};
	bool isNull() { return poly == NULL || poly->size() == 0; };
	void splice(PolygonPartReference& other, bool end, bool deleteOther) {
		auto pos = end ? poly->end() : poly->begin();
		poly->splice(pos, *other.poly);
		if (deleteOther) {
			if (end) {
				auto current = &other;
				while (current != NULL) {
					current->poly = NULL;
					current = current->rightSibling;
				}
			}
			else {
				auto current = &other;
				while (current != NULL) {
					current->poly = NULL;
					current = current->leftSibling;
				}
			}
		}
		else {
			if (end) {
				rightSibling = &other;
				other.leftSibling = this;
				auto current = &other;
				while (current != NULL) {
					current->poly = poly;
					current = current->rightSibling;
				}
			}
			else {
				leftSibling = &other;
				other.rightSibling = this;
				auto current = &other;
				while (current != NULL) {
					current->poly = poly;
					current = current->leftSibling;
				}
			}
		}
	}
};

struct PolySweepEdge;

struct PolySweepEvent {
	PolySweepEventType type;
	Point64 location;
	PolySweepEvent* prev;
	PolySweepEvent* next;

	//only for intersections
	PolySweepEdge* left = NULL;
	PolySweepEdge* right = NULL;
};

struct PolySweepEdge {
	PolySweepEdgeType type;
	Point64 location;
	PolySweepEvent* prev;
	PolySweepEvent* next;
	PolygonPartReference* leftPoly = NULL;
	PolygonPartReference* rightPoly = NULL;
	PolySweepEvent* bottom = NULL;
	bool isSubject() {
		return (type == PolySweepEdgeType::SUBJECT_POINT_LEFT || type == PolySweepEdgeType::SUBJECT_POINT_RIGHT);
	}
};


// Function to find orientation of the ordered triplet (p, q, r)
// 0 -> p, q and r are collinear
// 1 -> Clockwise
// 2 -> Counterclockwise
static int orientation(Point64 p, Point64 q, Point64 r) {
	double val = (q.y - p.y) * (r.x - q.x) -
		(q.x - p.x) * (r.y - q.y);
	if (val == 0) return 0;  // collinear
	return (val > 0) ? 1 : 2; // clock or counterclock wise
}

// Function to check if point q lies on line segment 'pr'
static bool onSegment(Point64 p, Point64 q, Point64 r) {
	if (q.x <= std::max(p.x, r.x) && q.x >= std::min(p.x, r.x) &&
		q.y <= std::max(p.y, r.y) && q.y >= std::min(p.y, r.y))
		return true;
	return false;
}

// Function to check if two given line segments intersect
static bool findIntersection(const Point64& p1, const Point64& q1, const Point64& p2, const Point64& q2, Point64& intersection) {
	//if edges share same bottom point ignore intersection
	if (q1 == q2 || p1 == p2) {
		return false;
	}

	// Find the four orientations needed for the general and
	// special cases
	int o1 = orientation(p1, q1, p2);
	int o2 = orientation(p1, q1, q2);
	int o3 = orientation(p2, q2, p1);
	int o4 = orientation(p2, q2, q1);

	// General case
	if (o1 != o2 && o3 != o4) {
		// Calculate intersection point
		double a1 = q1.y - p1.y;
		double b1 = p1.x - q1.x;
		double c1 = a1 * p1.x + b1 * p1.y;

		double a2 = q2.y - p2.y;
		double b2 = p2.x - q2.x;
		double c2 = a2 * p2.x + b2 * p2.y;

		double determinant = a1 * b2 - a2 * b1;

		if (determinant != 0) {
			intersection.x = (b2 * c1 - b1 * c2) / determinant;
			intersection.y = (a1 * c2 - a2 * c1) / determinant;
		}

		return true;
	}
	/*
	// Special cases
	// p1, q1 and p2 are collinear and p2 lies on segment p1q1
	if (o1 == 0 && onSegment(p1, p2, q1)) return true;

	// p1, q1 and q2 are collinear and q2 lies on segment p1q1
	if (o2 == 0 && onSegment(p1, q2, q1)) return true;

	// p2, q2 and p1 are collinear and p1 lies on segment p2q2
	if (o3 == 0 && onSegment(p2, p1, q2)) return true;

	// p2, q2 and q1 are collinear and q1 lies on segment p2q2
	if (o4 == 0 && onSegment(p2, q1, q2)) return true;*/

	return false; // Doesn't fall in any of the above cases
}

static void createIntersectionEvent(const std::list<PolySweepEdge*>& status, std::list<PolySweepEvent*>& events, PolySweepEdge* topLeft, PolySweepEdge* topRight, Point64& intersectionPoint) {
	auto it = events.rbegin();
	while (it != events.rend()) {
		if (intersectionPoint.y > (*it)->location.y || intersectionPoint.y == (*it)->location.y && intersectionPoint.x < (*it)->location.x
			|| (*it) == topLeft->bottom || (*it) == topRight->bottom || (*it)->right == topRight || (*it)->left == topRight || (*it)->right == topLeft || (*it)->left == topLeft) {
			break;
		}
		++it;
	}
	//avoid adding duplicate intersection
	if ((*it)->type == PolySweepEventType::INTERSECTION && (*it)->left == topLeft && (*it)->right == topRight) {
		return;
	}
	auto intersectionEvent = new PolySweepEvent({ PolySweepEventType::INTERSECTION, intersectionPoint, NULL, NULL, topLeft, topRight });
	events.insert(it.base(), intersectionEvent);
}

static Paths difference(const Paths& subject, Paths& clip) {
	Paths result;

	std::list<std::list<Point64>> polygonParts;
	std::list<PolySweepEvent*> events;
	std::list<PolySweepEdge*> status;

	std::vector<PolySweepEvent*> eventGarbage;
	std::vector<PolySweepEdge*> edgeGarbage;

	for (auto& path : subject) {
		PolySweepEvent* prev = nullptr;
		PolySweepEvent* first;
		for (auto& p : path) {
			auto newEvent = new PolySweepEvent({ PolySweepEventType::SUBJECT_POINT, p, prev, nullptr });
			if (prev != nullptr)
				prev->next = newEvent;
			else
				first = newEvent;
			prev = newEvent;
			events.push_back(newEvent);
		}
		events.back()->next = first;
		first->prev = events.back();
	}
	for (auto& path : clip) {
		PolySweepEvent* prev = nullptr;
		PolySweepEvent* first;
		for (auto& p : path) {
			auto newEvent = new PolySweepEvent({ PolySweepEventType::CLIP_POINT, p, prev, nullptr });
			if (prev != nullptr)
				prev->next = newEvent;
			else
				first = newEvent;
			prev = newEvent;
			events.push_back(newEvent);
		}
		events.back()->next = first;
		first->prev = events.back();
	}

	events.sort( [](const PolySweepEvent* A, const PolySweepEvent* B) -> bool {
			return A->location.y < B->location.y || (A->location.y == B->location.y && A->location.x > B->location.x);
		});

	while (!events.empty()) {
		auto current = events.back();
		auto statusIt = status.begin();
		bool insideSubject = false;
		size_t clipDepth = 0;
		PointOnLineResult eventLocation = PointOnLineResult::RIGHT;
		do
		{
			current = events.back();
			events.pop_back();
			//find position relative to first status edge
			if (statusIt != status.end() && current->type != PolySweepEventType::INTERSECTION) {
				eventLocation = PointLineLocation((*statusIt)->location, (*statusIt)->bottom->location, current->location);
			}

			//find point location inside status
			while (eventLocation == PointOnLineResult::RIGHT && statusIt != status.end() && current->type != PolySweepEventType::INTERSECTION) {
				if ((*statusIt)->isSubject()) {
					insideSubject = !insideSubject;
				}
				else if ((*statusIt)->type == PolySweepEdgeType::CLIP_POINT_LEFT){
					clipDepth++;
				}
				else {
					clipDepth--;
				}
				statusIt++;
				if (statusIt == status.end()) {
					break;
				}
				eventLocation = PointLineLocation((*statusIt)->location, (*statusIt)->bottom->location, current->location);
			}

			Point64 intersection;

			//Point is to the right of entire status
			if (eventLocation == PointOnLineResult::RIGHT && current->type != PolySweepEventType::INTERSECTION) {
				auto statusLeft = new PolySweepEdge({ PolySweepEdgeType::NONE, current->location, current->prev, current->next });
				auto statusRight = new PolySweepEdge({ PolySweepEdgeType::NONE, current->location, current->prev, current->next });
				statusLeft->bottom = current->next;
				statusRight->bottom = current->prev;
				Point64 intersectionResult;
				//new point is part of subject
				if (current->type == PolySweepEventType::SUBJECT_POINT) {
					statusLeft->type = PolySweepEdgeType::SUBJECT_POINT_LEFT;
					statusRight->type = PolySweepEdgeType::SUBJECT_POINT_RIGHT;
					if (status.size() > 0 && status.back()->type == PolySweepEdgeType::CLIP_POINT_RIGHT) {
						if (findIntersection(current->location, current->next->location, status.back()->location, status.back()->prev->location, intersectionResult)) {
							createIntersectionEvent(status, events, status.back(), statusLeft, intersectionResult);
						}
					}
				}
				//new point is part of clip
				else if (current->type == PolySweepEventType::CLIP_POINT) {
					statusLeft->type = PolySweepEdgeType::CLIP_POINT_LEFT;
					statusRight->type = PolySweepEdgeType::CLIP_POINT_RIGHT;
					if (status.size() > 0) {
						if (findIntersection(current->location, current->next->location, status.back()->location, status.back()->prev->location, intersectionResult)) {
							createIntersectionEvent(status, events, status.back(), statusLeft, intersectionResult);
						}
					}
				}
				//new point is intersection of previous edges
				else {
					throw "should not reach here";
				}
				if (current->type == PolySweepEventType::SUBJECT_POINT && clipDepth == 0 || current->type == PolySweepEventType::CLIP_POINT && clipDepth == 0 && insideSubject) {
					polygonParts.push_back({ current->location });
					auto polyRef = new PolygonPartReference(&polygonParts.back());
					statusLeft->rightPoly = polyRef;
					statusRight->leftPoly = polyRef;
				}
				status.push_back(statusLeft);
				status.push_back(statusRight);
			}

			//point is between two edges
			else if (eventLocation == PointOnLineResult::LEFT && current->type != PolySweepEventType::INTERSECTION) {
				auto statusLeft = new PolySweepEdge({ PolySweepEdgeType::NONE, current->location, current->prev, current->next });
				auto statusRight = new PolySweepEdge({ PolySweepEdgeType::NONE, current->location, current->prev, current->next });
				
				Point64 intersectionResult;
				//point is inside subject
				if (insideSubject) {
					statusLeft->bottom = current->prev;
					statusRight->bottom = current->next;
					//point is part of subject
					if (current->type == PolySweepEventType::SUBJECT_POINT) {
						statusLeft->type = PolySweepEdgeType::SUBJECT_POINT_LEFT;
						statusRight->type = PolySweepEdgeType::SUBJECT_POINT_RIGHT;

						if (clipDepth == 0) {
							polygonParts.push_back({ current->location });
							auto polyRef = new PolygonPartReference(&polygonParts.back());
							statusLeft->rightPoly = polyRef;
							statusRight->leftPoly = polyRef;
							statusLeft->leftPoly = (*std::prev(statusIt))->rightPoly;
							statusRight->rightPoly = (*statusIt)->leftPoly;
						}

						//look for left intersection
						if (statusIt != status.begin()) {
							if (findIntersection(current->location, statusLeft->bottom->location, (*std::prev(statusIt))->location, (*std::prev(statusIt))->bottom->location, intersectionResult)) {
								createIntersectionEvent(status, events, *std::prev(statusIt), statusLeft, intersectionResult);
							}
						}
						//look for right intersection
						Point64 bottomRight = current->prev->location;
						if (findIntersection(current->location, statusRight->bottom->location, (*statusIt)->location, (*statusIt)->bottom->location, intersectionResult)) {
							createIntersectionEvent(status, events, statusRight, *statusIt, intersectionResult);
						}

						insideSubject = !insideSubject;
					}
					//point is part of clip
					else {
						statusLeft->type = PolySweepEdgeType::CLIP_POINT_LEFT;
						statusRight->type = PolySweepEdgeType::CLIP_POINT_RIGHT;
						statusLeft->bottom = current->next;
						statusRight->bottom = current->prev;
						if (statusIt != status.begin()) {
							//check left intersection
							if (findIntersection(current->location, current->next->location, (*std::prev(statusIt))->location, (*std::prev(statusIt))->bottom->location, intersectionResult)) {
								createIntersectionEvent(status, events, *std::prev(statusIt), statusLeft, intersectionResult);
							}
						}
						//check right intersection
						if (findIntersection(current->location, current->prev->location, (*statusIt)->location, (*statusIt)->bottom->location, intersectionResult)) {
							createIntersectionEvent(status, events, statusRight, *statusIt, intersectionResult);
						}
						//create new hole
						if (clipDepth == 0 && insideSubject) {
							polygonParts.push_back({ current->location });
							auto polyRef = new PolygonPartReference(&polygonParts.back());
							statusLeft->rightPoly = polyRef;
							statusRight->leftPoly = polyRef;
							statusLeft->leftPoly = (*std::prev(statusIt))->rightPoly;
							statusRight->rightPoly = (*statusIt)->leftPoly;
						}
						clipDepth++;
					}
				}
				//point is outside subject
				else {
					//point is part of subject
					if (current->type == PolySweepEventType::SUBJECT_POINT) {
						statusLeft->type = PolySweepEdgeType::SUBJECT_POINT_LEFT;
						statusRight->type = PolySweepEdgeType::SUBJECT_POINT_RIGHT;
						statusLeft->bottom = current->next;
						statusRight->bottom = current->prev;

						if (clipDepth == 0) {
							polygonParts.push_back({ current->location });
							auto polyRef = new PolygonPartReference(&polygonParts.back());
							statusLeft->rightPoly = polyRef;
							statusRight->leftPoly = polyRef;
						}

						//look for left intersection
						if (statusIt != status.begin()) {
							if (findIntersection(current->location, statusLeft->bottom->location, (*std::prev(statusIt))->location, (*std::prev(statusIt))->bottom->location, intersectionResult)) {
								createIntersectionEvent(status, events, *std::prev(statusIt), statusLeft, intersectionResult);
							}
						}
						//look for right intersection
						if (findIntersection(current->location, statusRight->bottom->location, (*statusIt)->location, (*statusIt)->bottom->location, intersectionResult)) {
							createIntersectionEvent(status, events, statusRight ,*statusIt, intersectionResult);
						}

						insideSubject = !insideSubject;
					}
					//point is part of clip
					else if (current->type == PolySweepEventType::CLIP_POINT) {
						statusLeft->bottom = current->next;
						statusRight->bottom = current->prev;
						//look for left intersection
						if (statusIt != status.begin()) {
							if (findIntersection(current->location, current->next->location, (*std::prev(statusIt))->location, (*std::prev(statusIt))->bottom->location, intersectionResult)) {
								createIntersectionEvent(status, events, *std::prev(statusIt), statusLeft, intersectionResult);
							}
						}
						//look for right intersection
						if (findIntersection(current->location, current->prev->location, (*statusIt)->location, (*statusIt)->bottom->location, intersectionResult)) {
							createIntersectionEvent(status, events, statusRight ,*statusIt, intersectionResult);
						}
						statusLeft->type = PolySweepEdgeType::CLIP_POINT_LEFT;
						
						statusRight->type = PolySweepEdgeType::CLIP_POINT_RIGHT;
						if (insideSubject && clipDepth == 0) {
							polygonParts.push_back({ current->location });
							auto polyRef = new PolygonPartReference(&polygonParts.back());
							statusLeft->rightPoly = polyRef;
							statusRight->leftPoly = polyRef;
						}
						clipDepth++;
					}
					//point is intersection of previous edges
					else {
						throw "should not reach here";
					}
				}
				statusIt = status.insert(statusIt, statusLeft);
				statusIt++;
				statusIt = status.insert(statusIt, statusRight);
			}

			//point is on a segment
			else if (eventLocation == PointOnLineResult::ON && current->type != PolySweepEventType::INTERSECTION) {
				Point64 intersectionResult;

				//point belongs to this edge
				if ((*statusIt)->bottom == current) {
					//point is on right subject boundary
					if ((*statusIt)->isSubject() && insideSubject) {

						auto newEdge = new PolySweepEdge({ PolySweepEdgeType::NONE, current->location, current->prev, current->next });
						newEdge->leftPoly = (*statusIt)->leftPoly;
						newEdge->rightPoly = (*statusIt)->rightPoly;
						newEdge->type = (*statusIt)->type;
						newEdge->bottom = current->prev;
						edgeGarbage.push_back(*statusIt);
						if (clipDepth == 0) {
							if ((*statusIt)->rightPoly != NULL && !(*statusIt)->rightPoly->isNull()) {
								(*statusIt)->rightPoly->poly->push_front(current->location);
							}
							else {
								(*statusIt)->leftPoly->poly->push_front(current->location);
							}
						}
						statusIt = status.erase(statusIt);

						if (statusIt != status.end() && (*statusIt)->isSubject() && (*statusIt)->bottom == current) {
							if (clipDepth == 0) {
								(*newEdge).leftPoly->splice(*(*statusIt)->rightPoly, false, false);
							}
							edgeGarbage.push_back(*statusIt);
							statusIt = status.erase(statusIt);
							//check for new intersection between left and right neighbors
							if (statusIt != status.begin() && statusIt != status.end()) {
								if (findIntersection((*std::prev(statusIt))->location, (*std::prev(statusIt))->bottom->location,
									(*statusIt)->location, (*statusIt)->bottom->location, intersectionResult)
									&& (intersectionResult.y < current->location.y || intersectionResult.y == current->location.y && intersectionResult.x > current->location.x)) {

									createIntersectionEvent(status, events, *std::prev(statusIt), *statusIt, intersectionResult);
								}
							}
						}
						else {
							statusIt = status.insert(statusIt, newEdge);
							//check for new intersections with left and right neighbors
							if (statusIt != status.begin()) {
								if (findIntersection(current->location, current->prev->location, (*std::prev(statusIt))->location, (*std::prev(statusIt))->bottom->location, intersectionResult)) {
									createIntersectionEvent(status, events, *std::prev(statusIt), *statusIt, intersectionResult);
								}
							}
							if (std::next(statusIt) != status.end()) {
								if (findIntersection(current->location, current->prev->location, (*std::next(statusIt))->location, (*std::next(statusIt))->bottom->location, intersectionResult)) {
									createIntersectionEvent(status, events, *statusIt, *std::next(statusIt), intersectionResult);
								}
							}
						}
					}
					//point is on left subject boundary
					else if ((*statusIt)->isSubject() && !insideSubject) {

						auto newEdge = new PolySweepEdge({ PolySweepEdgeType::NONE, current->location, current->prev, current->next });
						newEdge->leftPoly = (*statusIt)->leftPoly;
						newEdge->rightPoly = (*statusIt)->rightPoly;
						newEdge->type = (*statusIt)->type;
						newEdge->bottom = current->next;

						edgeGarbage.push_back(*statusIt);
						statusIt = status.erase(statusIt);
						if ((*statusIt)->isSubject() && (*statusIt)->bottom == current) {
							if (clipDepth == 0) {
								if (newEdge->leftPoly == NULL || newEdge->leftPoly->isNull()) {
									if ((*statusIt)->rightPoly != NULL && !(*statusIt)->rightPoly->isNull()) {
										(*statusIt)->rightPoly->poly->push_front(current->location);
										newEdge->rightPoly->splice(*(*statusIt)->rightPoly, true, true);
									}
									else {
										newEdge->rightPoly->poly->push_back(current->location);
									}
								}
								else {
									if ((*statusIt)->rightPoly != NULL && !(*statusIt)->rightPoly->isNull()) {
										(*statusIt)->rightPoly->poly->push_front(current->location);
										newEdge->leftPoly->splice(*(*statusIt)->rightPoly, true, false);
									}
									else {
										newEdge->leftPoly->poly->push_back(current->location);
										(*statusIt)->leftPoly->splice(*newEdge->leftPoly, false, true);
									}
								}
							}
							edgeGarbage.push_back(*statusIt);
							statusIt = status.erase(statusIt);
							//check for new intersection between left and right neighbors
							if (statusIt != status.begin() && statusIt != status.end()) {
								if (findIntersection((*std::prev(statusIt))->location, (*std::prev(statusIt))->bottom->location, (*statusIt)->location, (*statusIt)->bottom->location, intersectionResult)
									&& (intersectionResult.y < current->location.y || intersectionResult.y == current->location.y && intersectionResult.x > current->location.x)) {
									createIntersectionEvent(status, events, *std::prev(statusIt), *statusIt, intersectionResult);
								}
							}
						}
						else {
							if (clipDepth == 0) {
								if (newEdge->leftPoly == NULL || newEdge->leftPoly->isNull()) {
									newEdge->rightPoly->poly->push_back(current->location);
								}
								else {
									newEdge->leftPoly->poly->push_back(current->location);
								}
							}
							statusIt = status.insert(statusIt, newEdge);

							//check for new intersections with left and right neighbors
							if (statusIt != status.begin()) {
								if (findIntersection(current->location, current->next->location, (*std::prev(statusIt))->location, (*std::prev(statusIt))->bottom->location, intersectionResult)) {
									createIntersectionEvent(status, events, *std::prev(statusIt), *statusIt, intersectionResult);
								}
							}
							if (std::next(statusIt) != status.end()) {
								if (findIntersection(current->location, current->next->location, (*std::next(statusIt))->location, (*std::next(statusIt))->bottom->location, intersectionResult)) {
									createIntersectionEvent(status, events, *statusIt, *std::next(statusIt), intersectionResult);
								}
							}

						}
					}
					//point is on left clip boundary
					else if ((*statusIt)->type == PolySweepEdgeType::CLIP_POINT_LEFT) {
						if (clipDepth == 0) {
							if (insideSubject) {
								if ((*statusIt)->rightPoly != NULL && !(*statusIt)->rightPoly->isNull()) {
									(*statusIt)->rightPoly->poly->push_front(current->location);
								}
								else {
									(*statusIt)->leftPoly->poly->push_front(current->location);
								}
							}
						}
						auto newEdge = new PolySweepEdge({ PolySweepEdgeType::NONE, current->location, current->prev, current->next });
						newEdge->type = PolySweepEdgeType::CLIP_POINT_LEFT;
						newEdge->leftPoly = (*statusIt)->leftPoly;
						newEdge->rightPoly = (*statusIt)->rightPoly;
						newEdge->bottom = current->next;
						edgeGarbage.push_back(*statusIt);
						statusIt = status.erase(statusIt);
						if (statusIt != status.end() && (*statusIt)->type == PolySweepEdgeType::CLIP_POINT_RIGHT && (*statusIt)->prev == current) {
							edgeGarbage.push_back(*statusIt);
							statusIt = status.erase(statusIt);
							//check for new intersection between left and right neighbors
							if (statusIt != status.begin() && statusIt != status.end()) {
								if (findIntersection((*std::prev(statusIt))->location, (*std::prev(statusIt))->bottom->location, (*statusIt)->location, (*statusIt)->bottom->location, intersectionResult)
									&& (intersectionResult.y < current->location.y || intersectionResult.y == current->location.y && intersectionResult.x > current->location.x)) {
									createIntersectionEvent(status, events, *std::prev(statusIt), *statusIt, intersectionResult);
								}
							}
						}
						else {
							statusIt = status.insert(statusIt, newEdge);
							//check for new intersections with left and right neighbors
							if (statusIt != status.begin()) {
								if (findIntersection(current->location, current->next->location, (*std::prev(statusIt))->location, (*std::prev(statusIt))->bottom->location, intersectionResult)) {
									createIntersectionEvent(status, events, *std::prev(statusIt), *statusIt, intersectionResult);
								}
							}
							if (std::next(statusIt) != status.end()) {
								if (findIntersection(current->location, current->next->location, (*std::next(statusIt))->location, (*std::next(statusIt))->bottom->location, intersectionResult)) {
									createIntersectionEvent(status, events, *statusIt, *std::next(statusIt), intersectionResult);
								}
							}
						}

					}
					//point is on right clip boundary
					else {
						auto newEdge = new PolySweepEdge({ PolySweepEdgeType::NONE, current->location, current->prev, current->next });
						newEdge->type = PolySweepEdgeType::CLIP_POINT_RIGHT;
						newEdge->leftPoly = (*statusIt)->leftPoly;
						newEdge->rightPoly = (*statusIt)->rightPoly;
						newEdge->bottom = current->prev;
						edgeGarbage.push_back(*statusIt);
						statusIt = status.erase(statusIt);
						if (statusIt != status.end() && (*statusIt)->type == PolySweepEdgeType::CLIP_POINT_LEFT && (*statusIt)->next == current) {
							if (insideSubject && clipDepth == 1) {
								if (newEdge->leftPoly == NULL || newEdge->leftPoly->isNull()) {
									if ((*statusIt)->rightPoly != NULL && !(*statusIt)->rightPoly->isNull()) {
										(*statusIt)->rightPoly->poly->push_front(current->location);
										newEdge->rightPoly->splice(*(*statusIt)->rightPoly, true, true);
									}
									else {
										newEdge->rightPoly->poly->push_back(current->location);
									}
								}
								else {
									if ((*statusIt)->rightPoly != NULL && !(*statusIt)->rightPoly->isNull()) {
										(*statusIt)->rightPoly->poly->push_front(current->location);
										newEdge->leftPoly->splice(*(*statusIt)->rightPoly, true, false);
									}
									else {
										newEdge->leftPoly->poly->push_back(current->location);
										(*statusIt)->leftPoly->splice(*newEdge->leftPoly, false, true);
									}
								}
							}
							edgeGarbage.push_back(*statusIt);
							statusIt = status.erase(statusIt);
							//check for new intersection between left and right neighbors
							if (statusIt != status.begin() && statusIt != status.end()) {
								if (findIntersection((*std::prev(statusIt))->location, (*std::prev(statusIt))->bottom->location, (*statusIt)->location, (*statusIt)->bottom->location, intersectionResult)
									&& (intersectionResult.y < current->location.y || intersectionResult.y == current->location.y && intersectionResult.x > current->location.x)) {
									createIntersectionEvent(status, events, *std::prev(statusIt), *statusIt, intersectionResult);
								}
							}
						}
						else {
							if (insideSubject && clipDepth == 1) {
								if (newEdge->leftPoly != NULL && !newEdge->leftPoly->isNull()) {
									newEdge->leftPoly->poly->push_back(current->location);
								}
								else {
									newEdge->rightPoly->poly->push_back(current->location);
								}
							}
							statusIt = status.insert(statusIt, newEdge);
							//check for new intersections
							if (statusIt != status.begin()) {
								if (findIntersection(current->location, current->prev->location, (*std::prev(statusIt))->location,
									(*std::prev(statusIt))->bottom->location, intersectionResult)) {

									createIntersectionEvent(status, events, *std::prev(statusIt), *statusIt, intersectionResult);
								}
							}
							if (std::next(statusIt) != status.end()) {
								if (findIntersection(current->location, current->prev->location, (*std::next(statusIt))->location, (*std::next(statusIt))->bottom->location, intersectionResult)) {
									createIntersectionEvent(status, events, *statusIt, *std::next(statusIt), intersectionResult);
								}
							}
						}
					}
				}
				//point does not belong to this edge
				else {
					std::cout << "sdlasdfjkpdfj";
				}
			}

			//point is intersection of previous edges
			else {
				//find corresponding edges in status
				statusIt = status.begin();
				insideSubject = false;
				clipDepth = 0;
				while ((*statusIt) != current->left) {
					if ((*statusIt)->isSubject()) {
						insideSubject = !insideSubject;
					}
					else if ((*statusIt)->type == PolySweepEdgeType::CLIP_POINT_LEFT) {
						clipDepth++;
					}
					else {
						clipDepth--;
					}
					statusIt++;
				}
				
				//find left and right neighboring edges for intersection calculation
				
				if (statusIt != status.begin()) {
					auto leftNeighbor = *std::prev(statusIt);
					Point64 intersectionResult;
					if (findIntersection(current->right->location, current->right->bottom->location,
						leftNeighbor->location, leftNeighbor->bottom->location, intersectionResult)) {

						createIntersectionEvent(status, events, leftNeighbor, current->right, intersectionResult);
					}
				}

				if (std::next(statusIt, 2) != status.end()) {
					auto rightNeighbor = *std::next(statusIt, 2);

					Point64 intersectionResult;
					if (findIntersection(current->left->location, current->left->bottom->location, rightNeighbor->location, rightNeighbor->bottom->location, intersectionResult)) {
						createIntersectionEvent(status, events, current->left, rightNeighbor, intersectionResult);
					}
				}
				//swap edges in status
				std::swap(*statusIt, *(std::next(statusIt)));
				PolygonPartReference* oldCenter = current->left->rightPoly;
				if (oldCenter == NULL || oldCenter->isNull()){
					oldCenter = current->right->leftPoly;
				}
				current->right->leftPoly = current->left->leftPoly;
				current->left->rightPoly = current->right->rightPoly;
				current->left->leftPoly = NULL;
				current->right->rightPoly = NULL;

				//incoming left edge is subject edge
				if (current->left->isSubject()) {
					Point64 shiftedLocation = current->location;
					if (insideSubject) {
						if (current->left->location.y > current->left->bottom->location.y) {
							shiftedLocation.x -= 1;
						}
						if (current->left->location.x < current->left->bottom->location.x) {
							shiftedLocation.y -= 1;
						}
						else if (current->left->location.x < current->left->bottom->location.x) {
							shiftedLocation.y += 1;
						}
					}
					else {
						if (current->left->location.y > current->left->bottom->location.y) {
							shiftedLocation.x += 1;
						}
						if (current->left->location.x < current->left->bottom->location.x) {
							shiftedLocation.y += 1;
						}
						else if (current->left->location.x < current->left->bottom->location.x) {
							shiftedLocation.y -= 1;
						}
					}
					if (current->right->type == PolySweepEdgeType::CLIP_POINT_LEFT) {
						if (clipDepth == 0) {
							if (insideSubject) {
								if (oldCenter != NULL && !oldCenter->isNull()) {
									oldCenter->poly->push_front(shiftedLocation);
									current->right->rightPoly = oldCenter;
								}
								else {
									current->right->leftPoly->poly->push_front(shiftedLocation);
								}
							}
							else {
								
								if (current->left->rightPoly != NULL && !current->left->rightPoly->isNull()) {
									current->left->rightPoly->poly->push_front(shiftedLocation);
									if (current->right->leftPoly != NULL && !current->right->leftPoly->isNull()) {
										current->right->leftPoly->splice(*current->left->rightPoly, true, false);
									}
									else {
										oldCenter->splice(*current->left->rightPoly, true, true);
									}
								}
								else if (current->right->leftPoly != NULL && !current->right->leftPoly->isNull()) {
									current->right->leftPoly->poly->push_back(shiftedLocation);
									oldCenter->splice(*current->right->leftPoly, false, true);
								}
								else {
									oldCenter->poly->push_back(shiftedLocation);
								}
							}
						}
						clipDepth++;
					}
					else if (current->right->type == PolySweepEdgeType::CLIP_POINT_RIGHT) {
						if (clipDepth == 1) {
							if (insideSubject) {
								polygonParts.push_back({ shiftedLocation });
								auto polyRef = new PolygonPartReference(&polygonParts.back());
								current->left->leftPoly = polyRef;
								current->right->rightPoly = polyRef;
							}
							else
							{
								if (oldCenter != NULL && !oldCenter->isNull()) {
									oldCenter->poly->push_back(shiftedLocation);
									current->left->leftPoly = oldCenter;
								}
								else
								{
									current->left->rightPoly->poly->push_back(shiftedLocation);
								}
							}
						}
						clipDepth--;
					}
				}
				
				//incoming left edge is clip left
				else if (current->left->type == PolySweepEdgeType::CLIP_POINT_LEFT) {
					if (current->right->isSubject()) {
						Point64 shiftedLocation = current->location;
						if (clipDepth == 0) {
							if (insideSubject) {
								shiftedLocation.x -= 1;
								if (current->right->location.x < current->right->bottom->location.x) {
									shiftedLocation.y -= 1;
								}
								else if (current->right->location.x < current->right->bottom->location.x) {
									shiftedLocation.y += 1;
								}
								if (oldCenter != NULL && !oldCenter->isNull()) {
									oldCenter->poly->push_front(current->location);
									current->right->rightPoly = oldCenter;
								}
								else {
									current->right->leftPoly->poly->push_front(shiftedLocation);
								}
							}
							else {
								shiftedLocation.x += 1;
								if (current->right->location.x < current->right->bottom->location.x) {
									shiftedLocation.y += 1;
								}
								else if (current->right->location.x < current->right->bottom->location.x) {
									shiftedLocation.y += 1;
								}
								polygonParts.push_back({ shiftedLocation });
								auto polyRef = new PolygonPartReference(&polygonParts.back());
								current->left->leftPoly = polyRef;
								current->right->rightPoly = polyRef;
							}
						}
						insideSubject = !insideSubject;
					}
					else if (current->right->type == PolySweepEdgeType::CLIP_POINT_LEFT) {
						if (clipDepth == 0) {
							if (insideSubject) {
								current->right->leftPoly->poly->push_front(current->location);
							}
						}
						clipDepth++;
					}
					else if (current->right->type == PolySweepEdgeType::CLIP_POINT_RIGHT) {
						if (clipDepth == 1 && insideSubject) {
							polygonParts.push_back({ current->location });
							auto polyRef = new PolygonPartReference(&polygonParts.back());
							current->left->leftPoly = polyRef;
							current->right->rightPoly = polyRef;
						}
						clipDepth--;
					}
				}
				//incoming left edge is clip right
				else if (current->left->type == PolySweepEdgeType::CLIP_POINT_RIGHT) {
					if (current->right->isSubject()) {
						Point64 shiftedLocation = current->location;
						if (clipDepth == 1) {
							if (insideSubject) {
								shiftedLocation.x -= 1;
								if (current->right->location.x < current->right->bottom->location.x) {
									shiftedLocation.y -= 1;
								}
								else if (current->right->location.x < current->right->bottom->location.x) {
									shiftedLocation.y += 1;
								}
								if (current->right->leftPoly != NULL && !current->right->leftPoly->isNull()) {
									current->right->leftPoly->poly->push_back(shiftedLocation);
									if (current->left->rightPoly != NULL && !current->left->rightPoly->isNull()) {
										current->right->leftPoly->splice(*current->left->rightPoly, true, false);
									}
									else {
										oldCenter->splice(*current->right->leftPoly, false, true);
									}
								}
								else {
									oldCenter->poly->push_back(shiftedLocation);
									if (current->left->rightPoly != NULL && !current->left->rightPoly->isNull()) {
										oldCenter->splice(*current->left->rightPoly, true, true);
									}
								}
								current->left->rightPoly = NULL;
								current->right->leftPoly = NULL;
							}
							else {
								shiftedLocation.x += 1;
								if (current->right->location.x < current->right->bottom->location.x) {
									shiftedLocation.y += 1;
								}
								else if (current->right->location.x < current->right->bottom->location.x) {
									shiftedLocation.y += 1;
								}
								if (oldCenter != NULL && !oldCenter->isNull()) {
									oldCenter->poly->push_back(shiftedLocation);
									current->left->leftPoly = oldCenter;
								}
								else {
									current->left->rightPoly->poly->push_back(shiftedLocation);
								}
							}
						}
						insideSubject = !insideSubject;
					}
					else if (current->right->type == PolySweepEdgeType::CLIP_POINT_LEFT) {
						if (clipDepth == 1) {
							if (insideSubject) {
								oldCenter->poly->push_back(current->location);
							}
						}
						clipDepth++;
					}
					else if (current->right->type == PolySweepEdgeType::CLIP_POINT_RIGHT) {
						if (clipDepth == 1) {
							if (insideSubject) {
								current->left->rightPoly->poly->push_back(current->location);
							}
						}
						clipDepth--;
					}
				}
				statusIt++;
				if (!events.empty() && events.back()->location.x <= current->location.x) {
					break;
				}
			}
			eventGarbage.push_back(current);
		} while (!events.empty() && events.back()->location.y >= current->location.y);
	}

	for (auto& part : polygonParts) {
		if (part.size() > 0) {
			Path p = {};
			for (auto& point : part) {
				p.push_back(point);
			}
			result.push_back(p);
		}
	}

	for (auto pointer : eventGarbage)
	{
		delete pointer;
	}
	eventGarbage.clear();
	for (auto pointer : edgeGarbage)
	{
		delete pointer;
	}
	edgeGarbage.clear();
	return result;
}