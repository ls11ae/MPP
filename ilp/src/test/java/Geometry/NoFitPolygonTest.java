package Geometry;

import DataStructure.Point;
import DataStructure.Polygon;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Assertions;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class NoFitPolygonTest {

    @Test
    public void testSimpleConvexPolygons() {
        Point p1 = new Point(0, 0);
        Point p2 = new Point(3, 0);
        Point p3 = new Point(4, 2);
        Point p4 = new Point(3, 5);
        Point p5 = new Point(0, 4);
        Point p6 = new Point(2, 0);
        Point p7 = new Point(2, 3);
        Point p8 = new Point(0, 3);

        Polygon fixed = new Polygon(new Point[]{p1, p2, p3, p4, p5});
        Polygon rotating = new Polygon(new Point[]{p6, p7, p8});

        Polygon actualNFP = NoFitPolygon.createConvex(fixed, rotating);
        Point[] expectedPoints = new Point[]{new Point(-2, -3), new Point(3, -3), new Point(4, -1),
                new Point(3, 2), new Point(1, 5), new Point(-2, 4)};
        assertEquals(6, actualNFP.size);
        Assertions.assertArrayEquals(expectedPoints, actualNFP.points);
    }

    // special cases of NFP -> failed: Clipper doesn't return lines or points (holes are fine)
    @Test
    public void testRobustnessNFPIsLine() {
        Point p1 = new Point(0, 0);
        Point p2 = new Point(2, 0);
        Point p3 = new Point(2, 2);
        Point p4 = new Point(4, 2);
        Point p5 = new Point(4, 0);
        Point p6 = new Point(6, 0);
        Point p7 = new Point(6, 4);
        Point p8 = new Point(0, 4);
        Point p9 = new Point(0, 0);
        Point p10 = new Point(2, 0);
        Point p11 = new Point(2, 2);
        Point p12 = new Point(0, 2);

        Polygon fixed = new Polygon(new Point[]{p1, p2, p3, p4, p5, p6, p7, p8});
        Polygon rotating = new Polygon(new Point[]{p9, p10, p11, p12});

        Polygon[] nfp = NoFitPolygon.createNonConvex(fixed, rotating);
        assertEquals(2, nfp.length);
    }

    @Test
    public void testRobustnessNFPHasHole() {
        Point p1 = new Point(0, 0);
        Point p2 = new Point(4, 0);
        Point p3 = new Point(2, 4);
        Point p4 = new Point(7, 4);
        Point p5 = new Point(5, 0);
        Point p6 = new Point(10, 0);
        Point p7 = new Point(10, 6);
        Point p8 = new Point(0, 6);
        Point p9 = new Point(0, 0);
        Point p10 = new Point(2, 0);
        Point p11 = new Point(2, 2);
        Point p12 = new Point(0, 2);

        Polygon fixed = new Polygon(new Point[]{p1, p2, p3, p4, p5, p6, p7, p8});
        Polygon rotating = new Polygon(new Point[]{p9, p10, p11, p12});

        Polygon[] nfp = NoFitPolygon.createNonConvex(fixed, rotating);
        assertEquals(2, nfp.length);
    }

    @Test
    public void testRobustnessNFPIsPoint() {
        Point p1 = new Point(0, 0);
        Point p2 = new Point(2, 0);
        Point p3 = new Point(2, 1);
        Point p4 = new Point(1.5, 1);
        Point p5 = new Point(1.5, 3);
        Point p6 = new Point(3.5, 3);
        Point p7 = new Point(3.5, 1);
        Point p8 = new Point(3, 1);
        Point p9 = new Point(3, 0);
        Point p10 = new Point(5, 0);
        Point p11 = new Point(5, 4);
        Point p12 = new Point(0, 4);
        Point p13 = new Point(0, 0);
        Point p14 = new Point(2, 0);
        Point p15 = new Point(2, 2);
        Point p16 = new Point(0, 2);

        Polygon fixed = new Polygon(new Point[]{p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12});
        Polygon rotating = new Polygon(new Point[]{p13, p14, p15, p16});

        Polygon[] nfp = NoFitPolygon.createNonConvex(fixed, rotating);
        assertEquals(2, nfp.length);
    }
}
