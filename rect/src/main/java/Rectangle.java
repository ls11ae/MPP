/**
 * Class for representing a rectangle with all it's attributes. These are parts of an Item (rectilinear block)
 * Attributes:
 * - x: The x-coordinate of the rectangle
 * - y: The y-coordinate of the rectangle
 * - width: The width of the rectangle
 * - height: The height of the rectangle
 * - relativeX: The x-coordinate of the relative position of the rectangle to the Item it belongs too
 * - relativeY: The y-coordinate of the relative position of the rectangle to the Item it belongs too
 * - groupID: The index/identifier of the Item, the rectangle belongs too 
 * 
 * @author Alkan
 *
 */
public class Rectangle {
	long x, y;
	long width, height;
	long relativeX, relativeY;
	int groupID;
	
	// Constructor for creating a rectangle
	public Rectangle(long x, long y, long w, long h , int id) {
		this.x = x;
		this.y = y;
		this.width = w;
		this.height = h;
		this.groupID = id;
		
		this.relativeX = x;
		this.relativeY = y;
	}
	
	// Constructor for creating a DeepCopy of a rectangle
    public Rectangle(Rectangle other) {
        this.x = other.x;
        this.y = other.y;
        this.width = other.width;
        this.height = other.height;
        this.relativeX = other.relativeX;
        this.relativeY = other.relativeY;
        this.groupID = other.groupID;
    }
    
    @Override
    public String toString() {
        return "Rectangle{" +
                "x=" + x +
                ", y=" + y +
                ", width=" + width +
                ", height=" + height +
                ", relativeX=" + relativeX +
                ", relativeY=" + relativeY +
                ", groupID=" + groupID +
                '}';
    }
	
}
