import java.util.ArrayList;
import java.util.List;

import clipper2.core.Paths64;

/**
 * Class for representing an Item/a Block. The Item is decomposed in it's rectangles.
 * Attributes:
 * - x: The x-coordinate of the block
 * - y: The y-coordinate of the block
 * - width: The width of the block
 * - height: The height of the block
 * - value: The value of the block
 * - id: The index/identifier of the block 
 * 
 * - rectangles: A list of rectangles, the block consists of
 * - nfp: The No-Fit-Polygon of the block with every already placed blocks and the container walls. (It continuously gets updated in the algorithm and contains
 * 			only the polygon inside the container.
 * 
 * @author Alkan
 *
 */
public class Item {
	int x, y;
	long width, height;
	long area;
	long value;
	int id;
	
	List<Rectangle> rectangles = new ArrayList<>();
	Paths64 nfp;
	
	// Constructor for creating an Item. (The rectangles of the decomposition are given as parameter)
	public Item(List<Rectangle> rects, long v, int id) {
		this.rectangles = rects;
		this.value = v;
		this.id = id;
		
		// Calculating the area of the item (sum of the areas of all rectangles it consists of)
		for(Rectangle rect : this.rectangles) {
			this.area += rect.width * rect.height;
		}
		
		// Calculating the width and height of the block
		for(Rectangle rect : rectangles) {
			if(rect.width + rect.relativeX > this.width) {
				this.width = rect.width + rect.relativeX;
			}
			if(rect.height + rect.relativeY > this.height) {
				this.height = rect.height + rect.relativeY;
			}
		}
		
	}
	
	// Constructor for creating a DeepCopy of an Item
	public Item(Item other) {
        this.x = other.x;
        this.y = other.y;
        this.width = other.width;
        this.height = other.height;
        this.area = other.area;
        this.value = other.value;
        this.id = other.id;
        
		this.rectangles = new ArrayList<>();
        for (Rectangle rect : other.rectangles) {
            this.rectangles.add(new Rectangle(rect));
        }

        this.nfp = other.nfp;
    }
	
}
