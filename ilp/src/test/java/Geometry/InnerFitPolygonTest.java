package Geometry;

import DataStructure.Point;
import DataStructure.Polygon;
import org.junit.jupiter.api.Test;

import java.util.ArrayList;
import java.util.Arrays;

import static org.junit.jupiter.api.Assertions.*;


public class InnerFitPolygonTest {

    @Test
    public void rectangleContainer() {
        //blaz3 instance
        Polygon container = new Polygon(new Point[]{new Point(0, 0), new Point(20, 0),
                new Point(20, 15), new Point(0, 15)});
        Polygon item3 = new Polygon(new Point[]{new Point(0, 0), new Point(2, 1),
                new Point(4, 0), new Point(3, 2), new Point(4, 5), new Point(2, 4),
                new Point(0, 5), new Point(1, 3)});
        Point[] expectedIFPPoints = new Point[]{new Point(0, 0), new Point(16, 0),
                new Point(16, 10), new Point(0, 10)};

        Polygon actualIFP = InnerFitPolygon.create(container, item3);
        assert actualIFP != null;
        ArrayList<Point> actualPoints = new ArrayList<>(Arrays.asList(actualIFP.points));
        assertEquals(expectedIFPPoints.length, actualIFP.size);
        for (Point expectedPoint : expectedIFPPoints) {
            assertTrue(actualPoints.contains(expectedPoint));
        }
        assertNull(InnerFitPolygon.create(item3, container));
    }

    @Test
    public void testInnerPolygons1() {
        //HS2 Figure 7 Example
        Point p1 = new Point(0, 2);
        Point p2 = new Point(1, 0);
        Point p3 = new Point(2, 0);
        Point p4 = new Point(3, 1);
        Point p5 = new Point(4, 1);
        Point p6 = new Point(5, 0);
        Point p7 = new Point(6, 2);
        Point p8 = new Point(6, 3);
        Point p9 = new Point(4, 4);
        Point p10 = new Point(1, 4);

        Polygon nfp = new Polygon(new Point[]{p1, p2, p3, p4, p5, p6, p7, p8, p9, p10});
        Polygon[] result = InnerFitPolygon.createInnerPolygons(nfp);
        assertEquals(1, result.length);

    }

    @Test
    public void testInnerPolygons2() {
        //HS2 Figure 8 Example
        Point p1 = new Point(0, 2);
        Point p2 = new Point(2, 2);
        Point p3 = new Point(2, 0);
        Point p4 = new Point(4, 0);
        Point p5 = new Point(4, 2);
        Point p6 = new Point(6, 2);
        Point p7 = new Point(6, 3);
        Point p8 = new Point(8, 3);
        Point p9 = new Point(8, 2);
        Point p10 = new Point(10, 2);
        Point p11 = new Point(10, 0);
        Point p12 = new Point(12, 0);
        Point p13 = new Point(12, 2);
        Point p14 = new Point(14, 2);
        Point p15 = new Point(14, 7);
        Point p16 = new Point(12, 7);
        Point p17 = new Point(12, 9);
        Point p18 = new Point(2, 9);
        Point p19 = new Point(2, 7);
        Point p20 = new Point(0, 7);


        Polygon nfp = new Polygon(new Point[]{p1, p2, p3, p4, p5, p6, p7, p8, p9, p10,
                p11, p12, p13, p14, p15, p16, p17, p18, p19, p20});
        Polygon[] result = InnerFitPolygon.createInnerPolygons(nfp);
        assertEquals(6, result.length);

    }
}
