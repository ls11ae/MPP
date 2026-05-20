package Geometry;

import DataStructure.Point;
import DataStructure.Polygon;
import Geometry.MCD.DecompPoly;
import Geometry.MCD.Diagonal;

import java.util.ArrayList;
import java.util.List;

/**
 * This class uses the Minimum Convex Decomposition algorithm by M. Keil and J. Snoeyink from:
 * On the time bound for convex decomposition of simple polygons.
 */
public class ConvexDecomposition {
    private static Polygon poly;            // cw version of simplePoly - used internally
    private static boolean reversed;     // true if the _internal_ indices have been reversed
    public static List<Diagonal> diagonals; // stores diagonals' indices (as found by _decompByDiags())


    /**
     * The MIT License (MIT)
     * <p>
     * Copyright (c) 2013-2016 Giuseppe Di Mauro (azrafe7)
     * Decomposes `simplePoly` into a minimum number of convex polygons.
     */
    public static Polygon[] decomposePoly(Polygon simplePoly) {
        List<Polygon> res = new ArrayList<>();

        List<List<Integer>> indices = decomposePolyIndices(simplePoly);

        for (List<Integer> polyIndices : indices) {
            ArrayList<Point> currPoly = new ArrayList<>();

            for (int idx : polyIndices) {
                currPoly.add(simplePoly.get(idx));
            }
            res.add(new Polygon(currPoly.toArray(Point[]::new)));
        }

        for (Polygon poly : res) {
            poly.removeCollinearPoints();
            poly.changeOrientationToCCW(true);
        }
        return res.toArray(Polygon[]::new);
    }

    /**
     * Decomposes `simplePoly` into a minimum number of convex polygons and returns their vertices' indices.
     */
    private static List<List<Integer>> decomposePolyIndices(Polygon simplePoly) {
        List<List<Integer>> res = new ArrayList<>();
        diagonals = new ArrayList<>();
        if (simplePoly.points.length < 3) return res;

        ArrayList<Point> polyL = new ArrayList<>();
        for (Point p : simplePoly.points) polyL.add(new Point(p.x, p.y));
        poly = new Polygon(polyL.toArray(Point[]::new));
        //reversed = poly.makeCW();    // make poly cw (in place)

        int i, j, k;
        int n = poly.points.length;
        DecompPoly decomp = new DecompPoly(poly);
        decomp.init();

        for (int l = 3; l <= n; l++) {
            i = decomp.reflexIter();

            while (i + l < n) {
                //trace("reflex: " + i + " vis:" + decomp.visible(i, i + l) + " " + poly.at(i));
                if (decomp.visible(i, k = i + l)) {
                    decomp.initPairs(i, k);
                    if (decomp.reflex(k)) {
                        for (j = i + 1; j < k; j++) decomp.typeA(i, j, k);
                    } else {
                        j = decomp.reflexIter(i + 1);
                        while (j < k - 1) {
                            decomp.typeA(i, j, k);
                            j = decomp.reflexNext(j);
                        }

                        decomp.typeA(i, k - 1, k); // do this, reflex or not.
                    }
                }

                i = decomp.reflexNext(i);
            }

            k = decomp.reflexIter(l);
            while (k < n) {

                if (!decomp.reflex(i = k - l) && decomp.visible(i, k)) {
                    decomp.initPairs(i, k);
                    decomp.typeB(i, i + 1, k); // do this, reflex or not.

                    j = decomp.reflexIter(i + 2);
                    while (j < k) {
                        decomp.typeB(i, j, k);
                        j = decomp.reflexNext(j);
                    }
                }

                k = decomp.reflexNext(k);
            }
        }
        decomp.guard = 3 * n;
        decomp.recoverSolution(0, n - 1);

        res = decomp.decompIndices();
/*
        if (reversed) {
            for (List<Integer> poly : res) {
                for (i = 0; i < poly.size(); i++) poly.set(i, n - poly.get(i) - 1);
            }
            for (Diagonal d : diagonals) {
                int tmp = d.from;
                d.from = n - d.to - 1;
                d.to = n - tmp - 1;
            }
        }

 */
        return res;
    }
}
