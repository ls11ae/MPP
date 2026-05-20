package ContinuousModels;

import DataStructure.*;
import Geometry.ConvexDecomposition;
import Geometry.InnerFitPolygon;
import Geometry.NoFitPolygon;
import Main.Main;
import com.gurobi.gurobi.*;

import java.io.File;
import java.util.ArrayList;

/**
 * This class contains methods that are used by the continuous models and
 * implements some constraints from the basic model.
 */
public abstract class ContinuousModel extends Model {
    protected Instance instance;
    GRBVar[][] xPos, yPos, packed;

    public ContinuousModel(ModelType type) throws GRBException {
        super(type);
    }

    /**
     * Creates the model for the instance by adding all constraints.
     */
    @Override
    public void createModel(Instance instance, boolean writeModelToFile) throws GRBException {
        this.instance = instance;
        createVariables();
        addAreaCut();
        addSymmetryBreaking();
        addInnerFitConstraints();
        addNonOverlappingConstraints();
        addObjective();
        model.update();
        if (writeModelToFile) {
            String path = "./Results/Files/Models/" + instance.type;
            new File(path).mkdirs();
            model.write(path + "/" + instance.instance_name + "_" + type + ".mps");
        }
    }

    @Override
    public Solution solveModel(String[] parameters, boolean writeModelSolutionToFile) throws GRBException {
        String path = "./Results/Files/Logs/" + instance.type;
        new File(path).mkdirs();
        model.set(GRB.StringParam.LogFile, path + "/" + instance.instance_name + "_" + type + ".log");
        for (String parameter : parameters) {
            String[] values = parameter.split("=");
            model.set(values[0], values[1]);
        }

        model.optimize();

        if (model.get(GRB.IntAttr.Status) == GRB.Status.INFEASIBLE) {
            System.out.println("Model is infeasible!");
            model.computeIIS();
            model.write("model.ilp");
            return null;
        } else {
            try {
                if (writeModelSolutionToFile) {
                    path = "./Results/Files/Solutions/" + instance.type;
                    new File(path).mkdirs();
                    model.write(path + "/" + instance.instance_name + "_" + type + ".sol");
                }
                return getSolution();
            } catch (GRBException e) {
                System.out.println("No solution found! Model status: " + model.get(GRB.IntAttr.Status));
                return null;
            }
        }
    }

    /**
     * This methode adds for each item the variables for the position in the container
     * and if it is packed to the model.
     */
    private void createVariables() throws GRBException {
        int n = instance.num_items;
        xPos = new GRBVar[n][];
        yPos = new GRBVar[n][];
        packed = new GRBVar[n][];

        for (int i = 0; i < n; i++) {
            Polygon item = instance.items[i];
            int quantity = item.quantity;
            xPos[i] = new GRBVar[quantity];
            yPos[i] = new GRBVar[quantity];
            packed[i] = new GRBVar[quantity];
            for (int j = 0; j < quantity; j++) {
                xPos[i][j] = model.addVar(0.0, instance.container.width - item.width,
                        0.0, GRB.CONTINUOUS, "x_" + i + "_" + j);
                yPos[i][j] = model.addVar(0.0, instance.container.height - item.height,
                        0.0, GRB.CONTINUOUS, "y_" + i + "_" + j);
                packed[i][j] = model.addVar(0, 1, 0.0, GRB.BINARY, "p_" + i + "_" + j);
            }
        }
    }


    /**
     * Adds the upper bound through the area to the model
     */
    private void addAreaCut() throws GRBException {
        GRBLinExpr areaExpr = new GRBLinExpr();
        GRBLinExpr sol = new GRBLinExpr();
        for (int i = 0; i < instance.num_items; i++) {
            for (int j = 0; j < instance.items[i].quantity; j++) {
                areaExpr.addTerm(instance.items[i].area, packed[i][j]);
                sol.addTerm(instance.items[i].value, packed[i][j]);
            }
        }

        model.addConstr(areaExpr, GRB.LESS_EQUAL, instance.container.area, "");
    }

    /**
     * Cherri et al. symmetry breaking constraints for multiple items of the same typ
     */
    private void addSymmetryBreaking() throws GRBException {
        // Cherri 3.1.3 Symmetry breaking multiple items
        GRBLinExpr expr;
        for (int i = 0; i < instance.num_items; i++) {
            Polygon item = instance.items[i];
            for (int j = 0; j < item.quantity - 1; j++) {
                // p_i_j >= p_i_j+1
                expr = new GRBLinExpr();
                expr.addTerm(-1.0, packed[i][j]);
                expr.addTerm(1.0, packed[i][j + 1]);
                model.addConstr(expr, GRB.LESS_EQUAL, 0.0, "");

                // x_i_j <= x_i_j+1 or y_i_j <= y_i_j+1
                GRBVar i_j;
                GRBVar i_j_1;
                if (type != ModelType.NFP_CM_VS) {
                    i_j = xPos[i][j];
                    i_j_1 = xPos[i][j + 1];
                } else {
                    // NFP-CM-VS uses the y coordinates
                    i_j = yPos[i][j];
                    i_j_1 = yPos[i][j + 1];
                }
                expr = new GRBLinExpr();
                expr.addTerm(1.0, i_j);
                expr.addTerm(-1.0, i_j_1);
                model.addConstr(expr, GRB.LESS_EQUAL, 0.0, "");
            }
        }
    }

    /**
     * Adds the inner-fit constraints for all items between the convex container
     * if an item has the same width or height as the container, then the solution is only valid
     * if the container is a rectangle (problems with clipper)
     */
    private void addInnerFitConstraints() throws GRBException {
        Polygon container = instance.container;
        for (int i = 0; i < instance.num_items; i++) {
            Polygon item = instance.items[i];
            Polygon ifp = InnerFitPolygon.create(container, item);

            for (int j = 0; j < item.quantity; j++) {
                // item doesn't fit or
                // problem with special cases with Clipper: IFP is point or line
                if (ifp == null) {
                    if (item.width > container.width || item.height > container.height) {
                        packed[i][j].set(GRB.DoubleAttr.UB, 0.0);
                    }
                    continue;
                }
                // Constraints: reference point is not on the right side for all edges (ab) in IFP
                // => the item is completely inside the container if packed
                for (int k = 0; k < ifp.size; k++) {

                    addLeftOnEdgeConstraint(item, ifp, k, packed[i][j], xPos[i][j], yPos[i][j]);
                }
            }
        }
    }

    abstract void addNonOverlappingConstraints() throws GRBException;

    /**
     * The objective function for the knapsack problem.
     */
    private void addObjective() throws GRBException {
        GRBLinExpr objective = new GRBLinExpr();
        for (int i = 0; i < instance.num_items; i++) {
            Polygon item = instance.items[i];
            for (int j = 0; j < item.quantity; j++) {
                objective.addTerm(item.value, packed[i][j]);
            }
        }
        model.setObjective(objective, GRB.MAXIMIZE);
    }

    /**
     * Retrieves the solution from the model after optimization.
     * @return best feasible solution
     */
    Solution getSolution() throws GRBException {
        int total_value = 0;
        int num_included_items = 0;
        ArrayList<Integer> item_indices = new ArrayList<>();
        ArrayList<Double> x_translation = new ArrayList<>();
        ArrayList<Double> y_translation = new ArrayList<>();
        for (int i = 0; i < instance.num_items; i++) {
            Polygon item = instance.items[i];
            for (int j = 0; j < item.quantity; j++) {
                if (packed[i][j].get(GRB.DoubleAttr.X) > 0.5) {
                    total_value += instance.items[i].value;
                    num_included_items++;
                    item_indices.add(instance.items[i].index);
                    x_translation.add(xPos[i][j].get(GRB.DoubleAttr.X) - item.xTranslation - instance.container.xTranslation);
                    y_translation.add(yPos[i][j].get(GRB.DoubleAttr.X) - item.yTranslation - instance.container.yTranslation);
                }
            }
        }
        return new Solution(instance.type, instance.instance_name, type,
                total_value, num_included_items, item_indices.stream().mapToInt(x -> x).toArray(),
                x_translation.stream().mapToDouble(x -> x).toArray(), y_translation.stream().mapToDouble(x -> x).toArray());
    }

    /**
     * Constraint to determine whether the reference point is not to the left of the edge
     * for the non-overlapping constraints
     */
    protected void addRightOnEdgeConstraint(Polygon fixed, Polygon rotating, Polygon nfp, int edgeIndex,
                                            GRBVar v, GRBVar x_i, GRBVar y_i, GRBVar x_j, GRBVar y_j) throws GRBException {
        Point a = nfp.get(edgeIndex);
        Point b = nfp.get(edgeIndex + 1);
        Point d = a.sub(b);
        double bigC = b.y * a.x - b.x * a.y;
//        double bigM = bigC + Math.abs(d.x) * instance.container.height +
//                Math.abs(d.y) * instance.container.width;
        double bigM = bigC;
        bigM += (instance.container.height - (d.x > 0 ? fixed.height : rotating.height)) * Math.abs(d.x);
        bigM += (instance.container.width - (d.y > 0 ? rotating.width : fixed.width)) * Math.abs(d.y);
        GRBLinExpr expr = new GRBLinExpr();
        expr.addTerm(-d.y, x_i);
        expr.addTerm(d.x, y_i);
        expr.addTerm(d.y, x_j);
        expr.addTerm(-d.x, y_j);
        expr.addTerm(bigM, v);
        model.addConstr(expr, GRB.LESS_EQUAL, bigM - bigC, "");
    }

    /**
     * Constraint to determine whether the reference point is not to the right of the edge
     * for the inner-fit constraints
     */
    protected void addLeftOnEdgeConstraint(Polygon item, Polygon polygon, int edgeIndex,
                                           GRBVar v, GRBVar x, GRBVar y) throws GRBException {
        addLeftOnEdgeConstraint(null, item, polygon, edgeIndex, v, null, null, x, y);

    }

    /**
     * Constraint to determine if the reference point is inside a convex area from filling the nfp or a hole
     * in the HS-Model (for every edge)
     */
    protected void addLeftOnEdgeConstraint(Polygon itemI, Polygon itemJ, Polygon polygon, int edgeIndex,
                                           GRBVar v, GRBVar x_i, GRBVar y_i, GRBVar x_j, GRBVar y_j) throws GRBException {
        Point a = polygon.get(edgeIndex);
        Point b = polygon.get(edgeIndex + 1);
        Point d = a.sub(b);
        double bigC = b.y * a.x - b.x * a.y;
//        double bigM = -bigC + Math.abs(d.x) * instance.container.height +
//                Math.abs(d.y) * instance.container.width;
        double bigM = -bigC;
        if (itemI == null) {
            bigM += Math.max(0, d.x * (instance.container.height - itemJ.height));
            bigM += Math.max(0, -d.y * (instance.container.width - itemJ.width));
        } else {
            bigM += (instance.container.height - (d.x > 0 ? itemJ.height : itemI.height)) * Math.abs(d.x);
            bigM += (instance.container.width - (d.y > 0 ? itemI.width : itemJ.width)) * Math.abs(d.y);
        }
        GRBLinExpr expr = new GRBLinExpr();
        if (x_i != null && y_i != null) {
            expr.addTerm(d.y, x_i);
            expr.addTerm(-d.x, y_i);
        }
        expr.addTerm(-d.y, x_j);
        expr.addTerm(d.x, y_j);
        expr.addTerm(bigM, v);
        model.addConstr(expr, GRB.LESS_EQUAL, bigC + bigM, "");
    }

    /**
     * Decomposes an item into disjoint convex parts
     * @return convex parts
     */
    protected Polygon[][] decomposePolygons() {
        Polygon[][] convexParts = new Polygon[instance.num_items][];
        for (int i = 0; i < instance.num_items; i++) {
            convexParts[i] = ConvexDecomposition.decomposePoly(instance.items[i]);
        }
        return convexParts;
    }


    /**
     * Adds the binary variable for each edge of each nfp to the model and returns them.
     */
    protected GRBVar[][] createEdgeVariables(Polygon[] nfps) throws GRBException {
        GRBVar[][] vars = new GRBVar[nfps.length][];
        boolean isNFP_CM_VS = type == ModelType.NFP_CM_VS;
        for (int m = 0; m < nfps.length; m++) {
            Polygon nfp = nfps[m];
            int n = nfp.size;
            if (isNFP_CM_VS) n += 2;
            vars[m] = new GRBVar[n];
            for (int e = 0; e < nfp.size; e++) {
                if (!isNFP_CM_VS || !isSideEdge(nfp.get(e), nfp.get(e + 1))) {
                    vars[m][e] = model.addVar(0.0, 1.0, 0.0, GRB.BINARY, "");

                }
            }
            // left and right special variable
            if (isNFP_CM_VS) {
                vars[m][n - 2] = model.addVar(0.0, 1.0, 0.0, GRB.BINARY, "");
                vars[m][n - 1] = model.addVar(0.0, 1.0, 0.0, GRB.BINARY, "");
            }
        }
        return vars;
    }

    protected boolean isSideEdge(Point p1, Point p2) {
        return Math.abs(p1.x - p2.x) <= Main.EPSILON;
    }

    protected Polygon[] createNFPs(Polygon[] convexPartsItemI, Polygon[] convexPartsItemJ) {
        Polygon[] nfps = new Polygon[convexPartsItemI.length * convexPartsItemJ.length];
        int index = 0;
        for (Polygon partI : convexPartsItemI) {
            for (Polygon partJ : convexPartsItemJ) {
                Polygon nfp = NoFitPolygon.createConvex(partI, partJ);
                nfp.setMinMaxX();
                nfps[index++] = nfp;
            }
        }
        return nfps;
    }

    /**
     * The constraint that ensures that exactly one variable of the nfp is 1, if both items are packed
     */
    protected void addSumConstraints(GRBVar[] vars, GRBVar p_i, GRBVar p_j) throws GRBException {
        GRBLinExpr sum1 = new GRBLinExpr();
        GRBLinExpr sum2 = new GRBLinExpr();
        sum1.addTerm(1.0, p_i);
        sum1.addTerm(1.0, p_j);
        for (GRBVar var : vars) {
            if (var != null) {
                sum1.addTerm(-1.0, var);
                sum2.addTerm(1.0, var);
            }
        }
        model.addConstr(sum2, GRB.LESS_EQUAL, p_i, "");
        model.addConstr(sum2, GRB.LESS_EQUAL, p_j, "");
        model.addConstr(sum1, GRB.LESS_EQUAL, 1.0, "");
    }

    protected void addDisjointDecompositionConstraint(GRBVar[] vars, GRBVar x_i, GRBVar x_j, GRBVar p_i, GRBVar p_j,
                                                   Polygon nfp, Polygon itemI, Polygon itemJ) throws GRBException {
        addDisjointDecompositionConstraint(vars, x_i, x_j, p_i, p_j, nfp, new Polygon[0], new Polygon[0], itemI, itemJ);
    }

    /**
     * Adds the constraints for partitioning the area outside the nfp into vertical slices in the NFP-CM-VS and HS-Model
     */
    public void addDisjointDecompositionConstraint(GRBVar[] vars, GRBVar x_i, GRBVar x_j, GRBVar p_i, GRBVar p_j,
                                                   Polygon nfp, Polygon[] innerPolygons, Polygon[] holes,
                                                   Polygon itemI, Polygon itemJ) throws GRBException {

        int left = nfp.size;
        int right = left + 1;
        double bigM1 = instance.container.width - itemI.width;
        double bigM2 = instance.container.width - itemJ.width;
        GRBLinExpr greater = new GRBLinExpr();
        GRBLinExpr lesser = new GRBLinExpr();
        greater.addTerm(bigM1, p_i);
        greater.addTerm(bigM1, p_j);
        lesser.addTerm(bigM2, p_i);
        lesser.addTerm(bigM2, p_j);
        greater.addTerm(1.0, x_i);
        greater.addTerm(-1.0, x_j);
        lesser.addTerm(-1.0, x_i);
        lesser.addTerm(1.0, x_j);
        greater.addTerm(-(instance.container.width - itemI.width), vars[left]);
        greater.addTerm(nfp.maxX, vars[right]);
        lesser.addTerm(-nfp.minX, vars[left]);
        lesser.addTerm(-(instance.container.width - itemJ.width), vars[right]);
        for (int i = 0; i < nfp.size; i++) {
            Point a = nfp.get(i);
            Point b = nfp.get(i + 1);
            if (!isSideEdge(a, b)) {
                GRBVar v = vars[i];
                // could be removed; elimination identical pieces
                if (v == null) continue;
                if (a.x < b.x) {
                    // bottom edge
                    greater.addTerm(a.x, v);
                    lesser.addTerm(-b.x, v);
                } else {
                    // top edge
                    greater.addTerm(b.x, v);
                    lesser.addTerm(-a.x, v);
                }
            }
        }
        // only for the HS-Model
        for (int i = 0; i < innerPolygons.length; i++) {
            Polygon inner = innerPolygons[i];
            GRBVar v = vars[nfp.size + 2 + i];
            greater.addTerm(inner.minX, v);
            lesser.addTerm(-inner.maxX, v);
        }
        for (int i = 0; i < holes.length; i++) {
            Polygon hole = holes[i];
            GRBVar v = vars[nfp.size + innerPolygons.length + 2 + i];
            greater.addTerm(hole.minX, v);
            lesser.addTerm(-hole.maxX, v);
        }

        model.addConstr(greater, GRB.LESS_EQUAL, 2.0 * bigM1, "");

        model.addConstr(lesser, GRB.LESS_EQUAL, 2.0 * bigM2, "");


    }
}
