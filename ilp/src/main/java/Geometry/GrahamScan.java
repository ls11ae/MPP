package Geometry;
/*
 * Copyright (c) 2010, Bart Kiers
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


import DataStructure.Point;
import DataStructure.Polygon;
import Main.Main;

import java.util.List;
import java.util.*;

/**
 *
 */
public final class GrahamScan {

    /**
     * An enum denoting a directional-turn between 3 points (vectors).
     */
    protected enum Turn { CLOCKWISE, COUNTER_CLOCKWISE, COLLINEAR }

    /**
     * Returns true iff all points in <code>points</code> are collinear.
     *
     * @param points the list of points.
     * @return       true iff all points in <code>points</code> are collinear.
     */
    private static boolean areAllCollinear(List<Point> points) {

        if(points.size() < 2) {
            return true;
        }

        final Point a = points.get(0);
        final Point b = points.get(1);

        for(int i = 2; i < points.size(); i++) {

            Point c = points.get(i);

            if(getTurn(a, b, c) != Turn.COLLINEAR) {
                return false;
            }
        }

        return true;
    }

    /**
     * Returns the convex hull of the points created from the list
     * <code>points</code>. Note that the first and last point in the
     * returned <code>List&lt;java.awt.Point&gt;</code> are the same
     * point.
     *
     * @param points the list of points.
     * @return       the convex hull of the points created from the list
     *               <code>points</code>.
     * @throws IllegalArgumentException if all points are collinear or if there
     *                                  are less than 3 unique points present.
     */
    public static Polygon getConvexHull(List<Point> points) throws IllegalArgumentException {

        List<Point> sorted = new ArrayList<>(getSortedPointSet(points));

        if(sorted.size() < 3) {
            throw new IllegalArgumentException("can only create a convex hull of 3 or more unique points");
        }

        if(areAllCollinear(sorted)) {
            throw new IllegalArgumentException("cannot create a convex hull from collinear points");
        }

        Stack<Point> stack = new Stack<>();
        stack.push(sorted.get(0));
        stack.push(sorted.get(1));

        for (int i = 2; i < sorted.size(); i++) {

            Point head = sorted.get(i);
            Point middle = stack.pop();
            Point tail = stack.peek();

            Turn turn = getTurn(tail, middle, head);

            switch(turn) {
                case COUNTER_CLOCKWISE:
                    stack.push(middle);
                    stack.push(head);
                    break;
                case CLOCKWISE:
                    i--;
                    break;
                case COLLINEAR:
                    stack.push(head);
                    break;
            }
        }

        // close the hull
//        stack.push(sorted.get(0));

        Polygon convexHull = new Polygon(stack.toArray(new Point[0]));
        convexHull.changeOrientationToCCW(true);
        return convexHull;
    }

    /**
     * Returns the points with the lowest y coordinate. In case more than 1 such
     * point exists, the one with the lowest x coordinate is returned.
     *
     * @param points the list of points to return the lowest point from.
     * @return       the points with the lowest y coordinate. In case more than
     *               1 such point exists, the one with the lowest x coordinate
     *               is returned.
     */
    private static Point getLowestPoint(List<Point> points) {

        Point lowest = points.get(0);

        for(int i = 1; i < points.size(); i++) {

            Point temp = points.get(i);

            if(temp.y - lowest.y < -Main.EPSILON ||
                    (Math.abs(temp.y - lowest.y) <= Main.EPSILON && temp.x - lowest.x < -Main.EPSILON)) {
                lowest = temp;
            }
        }

        return lowest;
    }

    /**
     * Returns a sorted set of points from the list <code>points</code>. The
     * set of points are sorted in increasing order of the angle they and the
     * lowest point <tt>P</tt> make with the x-axis. If tow (or more) points
     * form the same angle towards <tt>P</tt>, the one closest to <tt>P</tt>
     * comes first.
     *
     * @param points the list of points to sort.
     * @return       a sorted set of points from the list <code>points</code>.
     * @see GrahamScan#getLowestPoint(List)
     */
    private static Set<Point> getSortedPointSet(List<Point> points) {

        final Point lowest = getLowestPoint(points);

        TreeSet<Point> set = new TreeSet<>((a, b) -> {

            if (a == b || a.equals(b)) {
                return 0;
            }

            // use longs to guard against int-underflow
            double thetaA = Math.atan2(a.y - lowest.y, a.x - lowest.x);
            double thetaB = Math.atan2(b.y - lowest.y, b.x - lowest.x);

            if (thetaA - thetaB < -Main.EPSILON) {
                return -1;
            } else if (thetaA - thetaB > Main.EPSILON) {
                return 1;
            } else {
                // collinear with the 'lowest' point, let the point closest to it come first

                // use longs to guard against int-over/underflow
                double distanceA = Math.sqrt(((lowest.x - a.x) * (lowest.x - a.x)) +
                        ((lowest.y - a.y) * (lowest.y - a.y)));
                double distanceB = Math.sqrt(((lowest.x - b.x) * (lowest.x - b.x)) +
                        ((lowest.y - b.y) * (lowest.y - b.y)));

                if (distanceA - distanceB < -Main.EPSILON) {
                    return -1;
                } else {
                    return 1;
                }
            }
        });

        set.addAll(points);

        return set;
    }

    /**
     * Returns the GrahamScan#Turn formed by traversing through the
     * ordered points <code>a</code>, <code>b</code> and <code>c</code>.
     * More specifically, the cross product <tt>C</tt> between the
     * 3 points (vectors) is calculated:
     * <tt>(b.x-a.x * c.y-a.y) - (b.y-a.y * c.x-a.x)</tt>
     * and if <tt>C</tt> is less than 0, the turn is CLOCKWISE, if
     * <tt>C</tt> is more than 0, the turn is COUNTER_CLOCKWISE, else
     * the three points are COLLINEAR.
     *
     * @param a the starting point.
     * @param b the second point.
     * @param c the end point.
     * @return the GrahamScan#Turn formed by traversing through the
     *         ordered points <code>a</code>, <code>b</code> and
     *         <code>c</code>.
     */
    private static Turn getTurn(Point a, Point b, Point c) {

        // use longs to guard against int-over/underflow
        double crossProduct = ((b.x - a.x) * (c.y - a.y)) -
                ((b.y - a.y) * (c.x - a.x));

        if(crossProduct > Main.EPSILON) {
            return Turn.COUNTER_CLOCKWISE;
        }
        else if(crossProduct < -Main.EPSILON) {
            return Turn.CLOCKWISE;
        }
        else {
            return Turn.COLLINEAR;
        }
    }
}
