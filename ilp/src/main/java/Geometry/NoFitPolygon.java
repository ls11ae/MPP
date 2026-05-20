package Geometry;

import DataStructure.Point;
import DataStructure.Polygon;
import Main.Main;
import Other.Util;
import clipper2.Clipper;
import clipper2.core.FillRule;
import clipper2.core.PathsD;

import java.util.ArrayList;

public class NoFitPolygon {


    /**
     * Simple methode to calculate the convex nfp between two convex polygons.
     * @param fixed convex polygon with CCW orientation
     * @param rotating convex polygon with CCW orientation
     * @return convex nfp with CCW orientation
     */
    public static Polygon createConvex(Polygon fixed, Polygon rotating) {
        int startIndexFixed = findLowestPointIndex(fixed);
        int startIndexRotating = findHighestPointIndex(rotating);

        ArrayList<Point> nfp = new ArrayList<>();
        // starting at the reference point from fixed adapted to the reference point from rotating
        nfp.add(fixed.get(startIndexFixed).sub(rotating.get(startIndexRotating)));
        int i = 0, j = 0;
        while (i < fixed.size && j < rotating.size) {
            Point a = fixed.get(startIndexFixed + i + 1).sub(fixed.get(startIndexFixed + i));
            Point b = rotating.get(startIndexRotating + j).sub(rotating.get(startIndexRotating + j + 1));
            double cross = a.x * b.y - a.y * b.x;
            if(Math.abs(cross) <= Main.EPSILON) {
                nfp.add(nfp.getLast().add(a.add(b)));
                i++;
                j++;
            } else if (cross > Main.EPSILON) {
                nfp.add(nfp.getLast().add(a));
                i++;
            } else {
                nfp.add(nfp.getLast().add(b));
                j++;
            }
        }
        while (i < fixed.size) {
            nfp.add(nfp.getLast().add(fixed.get(startIndexFixed + i + 1).sub(fixed.get(startIndexFixed + i))));
            i++;
        }
        while (j < rotating.size) {
            nfp.add(nfp.getLast().add(rotating.get(startIndexRotating + j).sub(rotating.get(startIndexRotating + j + 1))));
            j++;
        }
        nfp.removeLast();
        return new Polygon(nfp.toArray(new Point[0]));
    }

    /**
     * Calculates the nfp between concave polygons by decomposing the polygons in convex pieces
     * and union the convex nfps between the convex pieces
     * @return concave nfp, not robust in special cases
     */
    public static Polygon[] createNonConvex(Polygon fixed, Polygon rotating) {
        Polygon[] convexPartsFixed = ConvexDecomposition.decomposePoly(fixed);
        Polygon[] convexPartsRotating = ConvexDecomposition.decomposePoly(rotating);
        if(convexPartsFixed.length == 1 && convexPartsRotating.length == 1) {
            return new Polygon[]{createConvex(fixed, rotating)};
        }
        PathsD union = new PathsD();
        for (Polygon fixedPart : convexPartsFixed) {
            for (Polygon rotatingPart : convexPartsRotating) {
                Polygon nfp = createConvex(fixedPart, rotatingPart);
                union = Clipper.Union(union, Util.PolygonToPathsD(nfp), FillRule.NonZero, 6);
            }
        }

        Polygon[] nfp = Util.PathsDToPolygon(union);
        for (Polygon part : nfp) {
            part.removeCollinearPoints();
            part.changeOrientationToCCW(false);
        }

        return nfp;
    }

    private static int findLowestPointIndex(Polygon polygon) {
        int index = 0;
        Point point = polygon.get(0);
        for (int i = 1; i < polygon.size; i++) {
            Point current = polygon.get(i);
            if (current.y < point.y ||
                    (current.y == point.y && current.x < point.x)) {
                index = i;
                point = current;
            }
        }
        return index;
    }

    private static int findHighestPointIndex(Polygon polygon) {
        int index = 0;
        Point point = polygon.get(0);
        for (int i = 1; i < polygon.size; i++) {
            Point current = polygon.get(i);
            if (current.y > point.y ||
                    (current.y == point.y && current.x > point.x)) {
                index = i;
                point = current;
            }
        }
        return index;
    }
}
