package DataStructure;

import Geometry.MCD.Homog;
import Main.Main;
import java.util.Locale;
import java.util.Objects;

/**
 * continuous point in the cartesian coordinate system
 */
public class Point {
    public double x, y;

    public Point(double x, double y) {
        this.x = x;
        this.y = y;
    }

    @Override
    public String toString() {
        return String.format(Locale.ENGLISH,"(%.3f, %.3f)", x, y);
//        return "(" + x + ", " + y + ")";
    }

    @Override
    public boolean equals(Object other) {
        if (other instanceof Point p) {
            return equals(p);
        }
        return super.equals(other);
    }

    public boolean equals(Point other) {
        return (Math.abs(x - other.x) <= Main.EPSILON) && (Math.abs(y - other.y) <= Main.EPSILON);
    }

    @Override
    public int hashCode() {
        return Objects.hash(x, y);
    }

    public Point add(Point other) {
        return new Point(x + other.x, y + other.y);
    }

    public Point sub(Point other) {
        return new Point(x - other.x, y - other.y);
    }


    //MinimumConvexDecomposition Point class methods
    private static double det3(Point p, Point q, Point r) {
        return (q.x - p.x) * (r.y - p.y) - (q.y - p.y) * (r.x - p.x);
    }

    private static boolean LEFT(double test) {
        return test > Main.EPSILON;
    }

    private static boolean ON(double test) {
        return Math.abs(test) <= Main.EPSILON;
    } //test == 0.0;

    private static boolean RIGHT(double test) {
        return test < -Main.EPSILON;
    }

    private static double dot(Point o, Point p, Point q) {
        return (p.x - o.x) * (q.x - o.x) + (p.y - o.y) * (q.y - o.y);
    }

    public static boolean leftORinside(Point p, Point q, Point r) {
        double test = det3(p, q, r);
        return (LEFT(test) || (ON(test) && dot(r, p, q) < 0.0));
    }

    public static boolean leftORonseg(Point p, Point q, Point r) {
        double test = det3(p, q, r);
        return (LEFT(test) || (ON(test) && dot(r, p, q) <= 0.0));
    }

    public static boolean right(Point p, Point q, Point r) {
        return RIGHT(det3(p, q, r));
    }

    public static boolean left(Point p, Point q, Point r) {
        return LEFT(det3(p, q, r));
    }

    public static boolean collinear(Point p, Point q, Point r) {
        return ON(det3(p, q, r));
    }

    public Homog meet(Point p) {
        return new Homog(x * p.y - y * p.x, y - p.y, p.x - x);
    }


    public static Point findIntersection(Point s1, Point e1, Point s2, Point e2) {
        //https://rosettacode.org/wiki/Find_the_intersection_of_two_lines#Java
        double a1 = e1.y - s1.y;
        double b1 = s1.x - e1.x;
        double c1 = a1 * s1.x + b1 * s1.y;

        double a2 = e2.y - s2.y;
        double b2 = s2.x - e2.x;
        double c2 = a2 * s2.x + b2 * s2.y;

        double delta = a1 * b2 - a2 * b1;
        return new Point((b2 * c1 - b1 * c2) / delta, (a1 * c2 - a2 * c1) / delta);
    }
}
