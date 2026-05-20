package Other;


import DataStructure.Instance;
import DataStructure.Solution;

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.geom.AffineTransform;


/**
 * Zoomable-Java-Panel that draws the solution.
 * Code from <a href="https://github.com/Thanasis1101/Zoomable-Java-Panel">github</a>
 */
public class Panel extends JPanel implements MouseWheelListener, MouseListener, MouseMotionListener {
    Instance instance;
    Solution solution;
    double zoomFactor = 1.0;
    double prevZoomFactor = 1.0;
    double xOffset = 100;
    double yOffset = 100;
    int xDiff, yDiff;
    boolean zoom, drag, release;
    Point startPoint;
    Polygon[] polygons;
    float[] alpha;
    Polygon container;


    public Panel(Instance instance, Solution solution) {
        super();
        this.instance = instance;
        this.solution = solution;
        polygons = createPolygons();
        addMouseWheelListener(this);
        addMouseListener(this);
        addMouseMotionListener(this);
    }

    public void drawSolution() {
        JFrame frame = new JFrame(solution.instance_name + "_sol_" + solution.total_value);
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.add(this);
        frame.setSize(800, 800);
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
    }

    public void paintComponent(Graphics g) {
        super.paintComponent(g);
        Graphics2D g2d = (Graphics2D) g;

        if (zoom) {
            AffineTransform at = new AffineTransform();

            double xRel = MouseInfo.getPointerInfo().getLocation().getX() - getLocationOnScreen().getX();
            double yRel = MouseInfo.getPointerInfo().getLocation().getY() - getLocationOnScreen().getY();

            double zoomDiv = zoomFactor / prevZoomFactor;

            xOffset = (zoomDiv) * (xOffset) + (1 - zoomDiv) * xRel;
            yOffset = (zoomDiv) * (yOffset) + (1 - zoomDiv) * yRel;

            at.translate(xOffset, yOffset);
            at.scale(zoomFactor, zoomFactor);
            prevZoomFactor = zoomFactor;
            g2d.transform(at);
            zoom = false;
        }
        if (drag) {
            AffineTransform at = new AffineTransform();
            at.translate(xOffset + xDiff, yOffset + yDiff);
            at.scale(zoomFactor, zoomFactor);
            g2d.transform(at);

            if (release) {
                xOffset += xDiff;
                yOffset += yDiff;
                drag = false;
            }

        }
        int i = 0;
        for (Polygon polygon : polygons) {
            g2d.setColor(new Color(1.0f, 0.0f, 0.0f, alpha[i]));
            i++;
            g2d.fill(polygon);
            g2d.setColor(Color.BLACK);
            g2d.draw(polygon);
        }
        g2d.setColor(Color.BLACK);
        g2d.draw(container);
    }

    private Polygon[] createPolygons() {

        DataStructure.Polygon polygon = instance.container;
        double scale = 1.0;
        if (polygon.width < 800 || polygon.height < 800) {
            scale = 800 / Math.min(polygon.width, polygon.height);
        }
        int n = polygon.size;
        int[] x = new int[n];
        int[] y = new int[n];
        DataStructure.Point point;
        for (int i = 0; i < n; i++) {
            point = instance.container.points[i];
            x[i] = (int) Math.round((point.x - instance.container.xTranslation) * scale);
            y[i] = (int) Math.round((polygon.height - point.y + instance.container.yTranslation) * scale);
        }
        container = new Polygon(x, y, n);

        double[] valueAreaRatios = getRatio();


        alpha = new float[solution.num_included_items];
        Polygon[] polygons = new Polygon[solution.num_included_items];
        for (int i = 0; i < solution.num_included_items; i++) {
            alpha[i] = (float) valueAreaRatios[solution.item_indices[i]];
            polygon = instance.items[solution.item_indices[i]];
            n = polygon.size;
            x = new int[n];
            y = new int[n];
            double xTranslation = solution.x_translations[i];
            double yTranslation = solution.y_translations[i];
            for (int j = 0; j < n; j++) {
                point = polygon.points[j];
                x[j] = (int) Math.round((point.x + polygon.xTranslation + xTranslation) * scale);
                y[j] = (int) Math.round((instance.container.height - point.y - polygon.yTranslation - yTranslation) * scale);
            }
            polygons[i] = new Polygon(x, y, n);
        }
        return polygons;
    }


    private double[] getRatio() {
        double[] ratios = new double[instance.num_items];
        double maxRatio = 0.0;
        for (int i = 0; i < ratios.length; i++) {
            ratios[i] = (double) instance.items[i].value / instance.items[i].area;
            if (ratios[i] > maxRatio) {
                maxRatio = ratios[i];
            }
        }
        for (int i = 0; i < ratios.length; i++) {
            ratios[i] /= maxRatio;
        }
        return ratios;
    }


    @Override
    public void mouseClicked(MouseEvent e) {
    }

    @Override
    public void mousePressed(MouseEvent e) {
        release = false;
        startPoint = MouseInfo.getPointerInfo().getLocation();
    }

    @Override
    public void mouseReleased(MouseEvent e) {
        release = true;
        repaint();
    }

    @Override
    public void mouseEntered(MouseEvent e) {

    }

    @Override
    public void mouseExited(MouseEvent e) {

    }

    @Override
    public void mouseDragged(MouseEvent e) {
        Point curPoint = e.getLocationOnScreen();
        xDiff = curPoint.x - startPoint.x;
        yDiff = curPoint.y - startPoint.y;

        drag = true;
        repaint();
    }

    @Override
    public void mouseMoved(MouseEvent e) {

    }

    @Override
    public void mouseWheelMoved(MouseWheelEvent e) {
        zoom = true;
        if (e.getWheelRotation() < 0) {
            zoomFactor *= 1.2;
            repaint();
        }
        if (e.getWheelRotation() > 0) {
            zoomFactor /= 1.2;
            repaint();
        }
    }
}

