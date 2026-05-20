package Geometry;

import DataStructure.Point;
import DataStructure.Polygon;
import Other.Util;
import clipper2.Clipper;
import clipper2.core.FillRule;
import clipper2.core.PathD;
import clipper2.core.PathsD;
import clipper2.core.PointD;

import java.util.*;

public class InnerFitPolygon {
    /**
     * Calculates the inner fit polygon between the container and the item
     * based on the Minkowski difference.
     *
     * @param container convex ccw polygon
     * @param item      simple ccw polygon
     * @return IFP or null if the item doesn't fit, not robust in special cases
     */
    public static Polygon create(Polygon container, Polygon item) {
        PathsD containerAsPathsD = Util.PolygonToPathsD(container);
        PathsD intersection = getShiftedContainer(containerAsPathsD, item.get(0));

        for (int i = 1; i < item.size; i++) {
            PathsD shiftedContainer = getShiftedContainer(containerAsPathsD, item.get(i));
            intersection = Clipper.Intersect(intersection, shiftedContainer, FillRule.NonZero, 6);
        }

        if (intersection.isEmpty()) {
            return null;
        }
        if (intersection.size() > 1) {
            System.out.println("The IFP from item " + item.index +
                    " is probably not correct (precision problems with Clipper).");
        }
        return Util.PathDToPolygon(intersection.getFirst());
    }

    private static PathsD getShiftedContainer(PathsD container, Point translation) {
        PathsD shiftedContainer = new PathsD();
        PathD pointDS = new PathD();
        for (PointD point : container.getFirst()) {
            pointDS.add(new PointD(point.x - translation.x, point.y - translation.y));
        }
        shiftedContainer.add(pointDS);
        return shiftedContainer;
    }


    /**
     * Calculates the convex areas to fill the nfp in the HS-Model from Alvarez et al.
     * @param nfp concave nfp
     * @return convex areas
     */
    public static Polygon[] createInnerPolygons(Polygon nfp) {
        ArrayList<Polygon> result = new ArrayList<>();
        innerPolygons(nfp, result);
        return result.toArray(new Polygon[0]);
    }

    private static void innerPolygons(Polygon nfp, ArrayList<Polygon> result) {
        if (nfp.isConvex()) {
            return;
        }

        // start at a convex point
        int n = nfp.size;
        int startIndex = 0;
        while (Point.right(nfp.get(startIndex - 1), nfp.get(startIndex), nfp.get(startIndex + 1))) {
            startIndex++;
        }
        ArrayList<ArrayList<Integer>> concaveAreas = new ArrayList<>();
        int index = startIndex;

        // find all concave areas
        for (int i = 0; i < n; i++) {

            if (Point.right(nfp.get(index), nfp.get(index + 1), nfp.get(index +2))) {
                ArrayList<Integer> concaveArea = new ArrayList<>();
                concaveArea.add(index);
                i++;
                index = (index + 1) % n;
                while (Point.right(nfp.get(index), nfp.get(index + 1), nfp.get(index +2))) {
                    concaveArea.add(index);
                    index = (index + 1) % n;
                    i++;
                }
                concaveArea.add(index);
                index = (index + 1) % n;
                concaveArea.add(index);
                concaveAreas.add(concaveArea);
            } else {
                index = (index + 1) % n;
            }

        }

        // close all disjoint concave areas sorted by size
        concaveAreas.sort(Comparator.comparingInt(x -> -x.size()));
        boolean[] used = new boolean[n];
        boolean[] remove = new boolean[n];
        int counter = 0;
        for (ArrayList<Integer> current : concaveAreas) {
            boolean disjoint = true;
            for (int num : current) {
                if (used[num]) {
                    disjoint = false;
                    break;
                }
            }
            if (disjoint) {
                Point[] points = new Point[current.size()];
                for (int j = 0; j < current.size(); j++) {
                    int num = current.get(j);
                    used[num] = true;
                    remove[num] = true;
                    counter++;
                    points[j] = nfp.get(num);
                }
                remove[current.getFirst()] = false;
                remove[current.getLast()] = false;
                counter -= 2;
                Polygon tmp = new Polygon(points);
                tmp.changeOrientationToCCW(true);
                tmp.setMinMaxX();
                result.add(tmp);

            }
        }
        Point[] newNFP = new Point[n - counter];
        int j = 0;
        for (int i = 0; i < n; i++) {
            if(!remove[i]) {
                newNFP[j] = nfp.get(i);
                j++;
            }
        }

        // until new NFP is convex
        Polygon next = new Polygon(newNFP);
        next.removeCollinearPoints();
        innerPolygons(next, result);
    }
}
