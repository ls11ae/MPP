import java.util.List;
import java.util.ArrayList;
import clipper2.core.Point64;

/**
 * Class for decomposing a block into rectangles. A simple method is used here which only works for sequential blocks (either h-sequential or v-sequential).
 * h-sequential: A block where no vertical line would cut the block into more than two parts
 * v-sequential: A block where no horizontal line would cut the block into more than two parts
 * This is sufficient for the instances of the CG:SHOP 2024 contest. Additionally the instances contain plus-blocks which contain one diagonal edge, which is specifically 
 * checked here for approximate these parts into rectangles. 
 * The algorithm works like a scan-line, so for a v-sequential block an imaginary horizontal line starts at the bottom and moves up, while cutting the block when corners are detected.
 * 
 * @author Alkan
 *
 */
public class BlockDecomposer {
	/**
	 * Method for decomposing the block into rectangles
	 * @param x: List of the x-coordinates in counter-clockwise order
	 * @param y: List of the y-coordinates in counter-clockwise order
	 * @param id: The index/identifier of the block
	 * @return List of the rectangles the block was decomposed in
	 */
	public static List<Rectangle> getRects(List<Long> x, List<Long> y, int id){
		int n = x.size();
		List<Rectangle> vSeqRectangles = new ArrayList<>();
		List<Rectangle> hSeqRectangles = new ArrayList<>();
		
		boolean hDiagPlus = false;		// A Plus-Block where a horizontal edge is diagonal instead
		boolean vDiagPlus = false;		// A Plus-Block where a vertical edge is diagonal instead
		boolean notVseq = false;		// true if the block is not V-Sequential
		boolean notHseq = false;		// true if the block is not H-Sequential
		//-----------------------------------------------------------------------------------------------------------------------
		// V-Sequential decomposing
		
		// Search the index of the bottom left corner (point with the smallest x among the smallest y coordinates)
		int bottomLeftIndex = 0;
		for(int i = 0; i < n; i++) {
			if(y.get(i) < y.get(bottomLeftIndex) || (y.get(i) == y.get(bottomLeftIndex) && x.get(i) < x.get(bottomLeftIndex))) {
				bottomLeftIndex = i;
			}
		}
		
		int leftIndex = bottomLeftIndex;
		int rightIndex = (bottomLeftIndex + 1) % n;
		
		Point64 leftPoint = new Point64(x.get(leftIndex), y.get(leftIndex));
		Point64 rightPoint = new Point64(x.get(rightIndex), y.get(rightIndex));
		
		// Cut off further rectangles until a break condition is reached
		while(true) {
			int nextLeftIndex = (leftIndex + (n - 1)) % n;		// leftIndex - 1
			int nextRightIndex = (rightIndex + 1) % n;
			
			Point64 nextLeftPoint = new Point64(x.get(nextLeftIndex), y.get(nextLeftIndex));
			Point64 nextRightPoint = new Point64(x.get(nextRightIndex), y.get(nextRightIndex));
			
			// Break Conditions:
			// 1. All rectangles are cut off correctly
			if((leftIndex + 1) % n == rightIndex && leftIndex != bottomLeftIndex) {
				break;
			}
			// 2. The block is not V-sequential, so the block isn't decomposed correctly (notVseq will be set on true, so that vSeqRectangles won't be used)
			if(nextLeftPoint.y <= leftPoint.y || nextRightPoint.y <= rightPoint.y) {
				notVseq = true;
				break;
			}
			
			// Check for the diagonal edge in the plus-blocks and adjust the coordinate for the approximation if needed.
			if(nextLeftPoint.x != leftPoint.x || nextRightPoint.x != rightPoint.x) {
				vDiagPlus = true;
				if(nextLeftPoint.x < leftPoint.x) {
					leftPoint.x = nextLeftPoint.x;
				}
				if(nextRightPoint.x > rightPoint.x) {
					rightPoint.x = nextRightPoint.x;
				}
			}
			
			long width = rightPoint.x - leftPoint.x;
			Rectangle rect = null;
			
			// Creating rectangle and updating indices and points. The imaginary scan-line is managed here, which cuts off the rectangles.
			if(nextLeftPoint.y < nextRightPoint.y) {
				long height = nextLeftPoint.y - leftPoint.y;
				rect = new Rectangle(leftPoint.x, leftPoint.y, width, height, id);
				
				leftIndex = (nextLeftIndex + (n - 1)) % n;		// leftIndex - 1
				leftPoint = new Point64(x.get(leftIndex), y.get(leftIndex));
				
				rightPoint.y += height;
				
			}else if(nextLeftPoint.y > nextRightPoint.y) {
				long height = nextRightPoint.y - rightPoint.y;
				rect = new Rectangle(leftPoint.x, leftPoint.y, width, height, id);
				
				rightIndex = (nextRightIndex + 1) % n;
				rightPoint = new Point64(x.get(rightIndex), y.get(rightIndex));
				
				leftPoint.y += height;
				
			}else if(nextLeftPoint.y == nextRightPoint.y) {
				long height = nextLeftPoint.y - leftPoint.y;
				rect = new Rectangle(leftPoint.x, leftPoint.y, width, height, id);
				
				leftIndex = (nextLeftIndex + (n - 1)) % n;		// leftIndex - 1
				rightIndex = (nextRightIndex + 1) % n;
				
				leftPoint = new Point64(x.get(leftIndex), y.get(leftIndex));
				rightPoint = new Point64(x.get(rightIndex), y.get(rightIndex));
			}
			
			if(rect != null) {
				vSeqRectangles.add(rect);
			}
			
		}
		
		//-----------------------------------------------------------------------------------------------------------------------
		// H-Sequential decomposing
		
		// Search the index of the top left corner (point with the highest y among the smallest x coordinates)
		int topLeftIndex = 0;
		for(int i = 0; i < n; i++) {
			if(x.get(i) < x.get(topLeftIndex) || (x.get(i) == x.get(topLeftIndex) && y.get(i) > y.get(topLeftIndex))) {
				topLeftIndex = i;
			}
		}

		int topIndex = topLeftIndex;
		int bottomIndex = (topLeftIndex + 1) % n;
		
		Point64 topPoint = new Point64(x.get(topIndex), y.get(topIndex));
		Point64 bottomPoint = new Point64(x.get(bottomIndex), y.get(bottomIndex));
		
		// Cut off further rectangles until a break condition is reached
		while(true) {
			int nextTopIndex = (topIndex + (n - 1)) % n;		// leftIndex - 1
			int nextBottomIndex = (bottomIndex + 1) % n;
			
			Point64 nextTopPoint = new Point64(x.get(nextTopIndex), y.get(nextTopIndex));
			Point64 nextBottomPoint = new Point64(x.get(nextBottomIndex), y.get(nextBottomIndex));
			
			// Break Conditions:
			// 1. All rectangles are cut off correctly
			if((topIndex + 1) % n == bottomIndex && topIndex != topLeftIndex) {
				break;
			}
			// 2. The block is not H-sequential, so the block isn't decomposed correctly (notHseq will be set on true, so that hSeqRectangles won't be used)
			if(nextTopPoint.x <= topPoint.x || nextBottomPoint.x <= bottomPoint.x) {
				notHseq = true;
				break;
			}
			
			// Check for the diagonal edge in the plus-blocks and adjust the coordinate for the approximation if needed.
			if(nextTopPoint.y != topPoint.y || nextBottomPoint.y != bottomPoint.y) {
				hDiagPlus = true;
				if(nextTopPoint.y > topPoint.y) {
					topPoint.y = nextTopPoint.y;
				}
				if(nextBottomPoint.y < bottomPoint.y) {
					bottomPoint.y = nextBottomPoint.y;
				}
			}
			
			long height = topPoint.y - bottomPoint.y;
			Rectangle rect = null;
			
			// Creating rectangle and updating indices and points. The imaginary scan-line is managed here, which cuts off the rectangles.
			if(nextTopPoint.x < nextBottomPoint.x) {
				long width = nextTopPoint.x - topPoint.x;
				rect = new Rectangle(bottomPoint.x, bottomPoint.y, width, height, id);
				
				topIndex = (nextTopIndex + (n - 1)) % n;		// leftIndex - 1
				topPoint = new Point64(x.get(topIndex), y.get(topIndex));
				
				bottomPoint.x += width;
				
			}else if(nextTopPoint.x > nextBottomPoint.x) {
				long width = nextBottomPoint.x - bottomPoint.x;
				rect = new Rectangle(bottomPoint.x, bottomPoint.y, width, height, id);
				
				bottomIndex = (nextBottomIndex + 1) % n;
				bottomPoint = new Point64(x.get(bottomIndex), y.get(bottomIndex));
				
				topPoint.x += width;
				
			}else if(nextTopPoint.x == nextBottomPoint.x) {
				long width = nextTopPoint.x - topPoint.x;
				rect = new Rectangle(bottomPoint.x, bottomPoint.y, width, height, id);
				
				topIndex = (nextTopIndex + (n - 1)) % n;		// leftIndex - 1
				bottomIndex = (nextBottomIndex + 1) % n;
				
				topPoint = new Point64(x.get(topIndex), y.get(topIndex));
				bottomPoint = new Point64(x.get(bottomIndex), y.get(bottomIndex));
			}
			
			if(rect != null) {
				hSeqRectangles.add(rect);
			}
			
		}
		//-----------------------------------------------------------------------------------------------------------------------
		// Choosing the list of rectangles which are used
		if(notHseq || vDiagPlus) {
			return vSeqRectangles;
		}
		if(notVseq || hDiagPlus) {
			return hSeqRectangles;
		}
		
		// It's both h-sequential and v-sequential and it's not a plus block, so the list with less rectangles is chosen
		if(hSeqRectangles.size() < vSeqRectangles.size()) {
			return hSeqRectangles;
		}else {
			return vSeqRectangles;
		}
		
	}
	
}
