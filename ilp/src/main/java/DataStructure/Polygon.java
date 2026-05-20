package DataStructure;

import java.util.ArrayList;

/**
 * This class represents the container and items of the instance,
 * but also the NFPs and IFPs.
 */
public class Polygon {
    public Point[] points;
    public int quantity, value, size, index;
    public double area, width, height, xTranslation, yTranslation;
    public double minX, maxX;

    public Polygon(Point[] points, int quantity, int value, int index) {
        this.points = points;
        this.quantity = quantity;
        this.value = value;
        this.index = index;
        size = points.length;
        area = getArea();
        normalize();
    }
    public Polygon(Point[] points) {
        this.points = points;
        size = points.length;
    }

    private double getArea() {
        double area = 0;
        for (int i = 0; i < size; i++) {
            Point p1 = get(i);
            Point p2 = get(i + 1);
            area += (p1.x - p2.x) * (p1.y + p2.y);
        }
        return Math.abs(area / 2);
    }

    public Point get(int i) {
        if (i < 0) {
            return points[(i + size) % size];
        } else {
            return points[i % size];
        }
    }

    public void setWidthAndHeight() {
        width = -Double.MAX_VALUE;
        height = -Double.MAX_VALUE;
        for (Point p : points) {
            if (p.x > width) {
                width = p.x;
            }
            if(p.y > height) {
                height = p.y;
            }
        }
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder("Polygon(");
        for (Point point : points) {
            sb.append(point).append(", ");
        }
        sb.deleteCharAt(sb.length() - 1);
        sb.deleteCharAt(sb.length() - 1);
        sb.append(')');
        return sb.toString();
    }

    private void normalize() {
        removeCollinearPoints();
        changeOrientationToCCW(false);
        shiftToZero();
        setWidthAndHeight();
    }

    public void removeCollinearPoints() {
        ArrayList<Point> noCollinearPoints = new ArrayList<>(size);
        boolean foundCollinearPoint = false;
        for (int i = 0; i < size; i++) {
            if (!Point.collinear(get(i - 1), get(i), get(i + 1))) {
                noCollinearPoints.add(get(i));
                continue;
            }
            foundCollinearPoint = true;
        }
        if (foundCollinearPoint) {
            points = noCollinearPoints.toArray(new Point[0]);
            size = points.length;
        }
    }

    public void changeOrientationToCCW(boolean isConvex) {
        if (!isCCW(isConvex)) {
            for (int i = 0; i < size / 2; i++) {
                Point tmp = points[i];
                points[i] = points[size - 1 - i];
                points[size - 1 - i] = tmp;
            }
        }
    }

    private boolean isCCW(boolean isConvex) {
        Point smallestPoint = points[0];
        int index = 0;
        if(!isConvex) {
            for (int i = 1; i < size; i++) {
                Point point = points[i];
                if (point.x < smallestPoint.x || point.x == smallestPoint.x && point.y < smallestPoint.y) {
                    smallestPoint = point;
                    index = i;
                }
            }
        }

        Point a = get(index - 1);
        Point b = smallestPoint;
        Point c = get(index + 1);
        return Point.left(a, b, c);
    }

    public boolean isConvex() {
        for (int i = 0; i < size; i++) {
            if (Point.right(get(i - 1), get(i), get(i +1))) {
                return false;
            }
        }
        return true;
    }

    private void shiftToZero() {
        xTranslation = Double.POSITIVE_INFINITY;
        yTranslation = Double.POSITIVE_INFINITY;
        for (Point point : points) {
            if (point.x <= xTranslation) {
                xTranslation = point.x;
            }
            if (point.y <= yTranslation) {
                yTranslation = point.y;
            }
        }
        for (Point point : points) {
            point.x -= xTranslation;
            point.y -= yTranslation;
        }
    }

    /**
     * Point-in-Polygon-Test after Jordan (pseudocode from wikipedia)
     */
    public boolean contains(Point p, boolean withoutBoundary) {
        int result = inside(p);
        if (withoutBoundary) {
            return result > 0;
        } else {
            return result >= 0;
        }
    }

    private int inside(Point p) {
        int t = -1;
        for (int i = 0; i < size; i++) {
            t *= crossProdTest(p, get(i), get(i + 1));
            if (t == 0) {
                break;
            }
        }
        return t;
    }

    private int crossProdTest(Point a, Point b, Point c) {
        if (a.y == b.y && a.y == c.y) {
            if ((b.x <= a.x && a.x <= c.x) || (c.x <= a.x && a.x <= b.x)) {
                return 0;
            } else {
                return 1;
            }
        }
        if (a.y == b.y && a.x == b.x) {
            return 0;
        }
        if (b.y > c.y) {
            Point tmp = b;
            b = c;
            c = tmp;
        }
        if (a.y <= b.y || a.y > c.y) {
            return 1;
        }
        double delta = (b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x);
        if (delta > 0) {
            return -1;
        } else if (delta < 0) {
            return 1;
        } else {
            return 0;
        }
    }

    public void setMinMaxX() {
        maxX = Double.NEGATIVE_INFINITY;
        minX = Double.POSITIVE_INFINITY;
        for (Point p : points) {
            if (p.x > maxX) {
                maxX = p.x;
            }
            if (p.x < minX) {
                minX = p.x;
            }
        }
    }
}
