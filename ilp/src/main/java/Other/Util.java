package Other;

import DataStructure.Point;
import DataStructure.Polygon;
import clipper2.core.PathD;
import clipper2.core.PathsD;
import clipper2.core.PointD;

/**
 * Class to switch between data structure for clipper
 */
public class Util {
    public static PathsD PolygonToPathsD(Polygon polygon) {
        PathD path = new PathD();
        for (Point point : polygon.points) {
            path.add(new PointD(point.x, point.y));
        }
        PathsD paths = new PathsD();
        paths.add(path);
        return paths;
    }

    public static Polygon[] PathsDToPolygon(PathsD pathsD) {
        Polygon[] polygons = new Polygon[pathsD.size()];
        for (int i = 0; i < pathsD.size(); i++) {
            polygons[i] = PathDToPolygon(pathsD.get(i));
        }
        return polygons;
    }

    public static Polygon PathDToPolygon(PathD path) {
        int n = path.size();
        Point[] points = new Point[n];
        for (int i = 0; i < n; i++) {
            PointD point = path.get(i);
            points[i] = new Point(point.x, point.y);
        }
        return new Polygon(points);
    }
}
