import clipper2.core.Point64;

/**
 * Simple class for saving a Point64, an Item, a distance (long) and a value-area-ratio (double). It's used to save the best Position of an Item for the Best-Fit algorithm 
 * with it's value-area-ratio and shortest distance to a container corner. 
 * Attributes:
 * - point: The "best" placing point for this item in the current iteration
 * - block: The item which belongs to the pair
 * - dist: The distance of the block to a corner if placed at point.
 * - ratio: Ratio of the value and the area of the block
 * 
 * @author Alkan
 *
 */
public class CandidateDetails {
	Point64 point;
	Item block;
	long dist;
	double ratio;
	
	// Constructor for creating a PointBlockDistTriple
	public CandidateDetails(Point64 point, Item block, long dist, double ratio) {
		this.point = point;
		this.block = block;
		this.dist = dist;
		this.ratio = ratio;
	}
	
}
