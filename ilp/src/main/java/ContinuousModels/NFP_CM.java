package ContinuousModels;

import DataStructure.*;
import com.gurobi.gurobi.GRB;
import com.gurobi.gurobi.GRBException;
import com.gurobi.gurobi.GRBLinExpr;
import com.gurobi.gurobi.GRBVar;
import java.util.ArrayList;

/**
 * This class implements the model based on the NoFitPolygon-CoveringModel (NFP-CM) by Cherri et al.
 * in Robust mixed-integer linear programming models for the irregular strip packing problem.
 * <a href="https://doi.org/10.1016/j.ejor.2016.03.009">DOI</a>
 */
public class NFP_CM extends ContinuousModel {
    public NFP_CM() throws GRBException {
        super(ModelType.NFP_CM);
    }

    /**
     * This methode adds the non-overlapping constraints between all items to the model
     * with the variable reduction and valid inequalities
     */

    protected void addNonOverlappingConstraints() throws GRBException {

        Polygon[][] convexParts = decomposePolygons();

        for (int i = 0; i < instance.num_items; i++) {
            Polygon itemI = instance.items[i];
            for (int j = i; j < instance.num_items; j++) {
                Polygon itemJ = instance.items[j];
                if (i == j && itemI.quantity == 1) {
                    continue;
                }

                // convex NFPs between the parts from item i and item j
                Polygon[] nfps = createNFPs(convexParts[i], convexParts[j]);

                ArrayList<int[]> collinearEdges = getCollinearEdges(nfps);

                // note that, k and l are for multiple items of the same typ and not the numbers for the convex parts
                for (int k = 0; k < itemI.quantity; k++) {
                    int start = i == j ? k + 1 : 0;
                    for (int l = start; l < itemJ.quantity; l++) {

                        // defines a binary variable for each edge of each convex nfp between item i and item j
                        GRBVar[][] vars = createEdgeVariables(nfps);

                        // Cherri 3.2.2 variable reduction
                        reduceVarsForCollinearEdges(collinearEdges, vars);

                        for (int m = 0; m < nfps.length; m++) {
                            Polygon nfp = nfps[m];

                            // at least one variable has to be 1 if both items are packed (equation (17))
                            addSumConstraints(vars[m], packed[i][k], packed[j][l]);

                            for (int e = 0; e < nfp.size; e++) {

                                // equation (16) in the paper
                                addRightOnEdgeConstraint(itemI, itemJ, nfp, e, vars[m][e],
                                        xPos[i][k], yPos[i][k], xPos[j][l], yPos[j][l]);

                            }
                        }
                        // Cherri et al. Algorithm 1: Valid inequalities
                        // but the algorithm is incorrect, only case d is valid!
                        addValidInequalities(nfps, vars);
                    }
                }
            }
        }
    }


    private ArrayList<int[]> getCollinearEdges(Polygon[] nfps) {
        ArrayList<int[]> collinearEdges = new ArrayList<>();
        for (int i = 0; i < nfps.length - 1; i++) {
            Polygon p = nfps[i];
            for (int j = i + 1; j < nfps.length; j++) {
                Polygon q = nfps[j];
                for (int k = 0; k < p.size; k++) {
                    // first directed edge (k1, k2)
                    Point k1 = p.get(k);
                    Point k2 = p.get(k + 1);
                    for (int e = 0; e < q.size; e++) {
                        // second directed edge (e1, e2)
                        Point e1 = q.get(e);
                        Point e2 = q.get(e + 1);
                        // both points from the second edge are collinear to the line through (k1, k2)
                        if (Point.collinear(k1, k2, e1) && Point.collinear(k1, k2, e2)) {
                            // both edges point in the same direction
                            if ((k1.x - k2.x) * (e1.x - e2.x) > 0 || (k1.y - k2.y) * (e1.y - e2.y) > 0) {
                                // edge k from nfp i and edge e from nfp j cover the same area and can be reduced
                                collinearEdges.add(new int[]{i, k, j, e});
                            }
                        }
                    }
                }
            }
        }
        return collinearEdges;
    }

    private void reduceVarsForCollinearEdges(ArrayList<int[]> collinearEdges, GRBVar[][] vars) throws GRBException {
        for (int[] e : collinearEdges) {
            GRBVar var1 = vars[e[0]][e[1]];
            GRBVar var2 = vars[e[2]][e[3]];
            // collinearEdges contains symmetrical edges
            // 1,2 and 1,3 collinear edges => 2,3 collinear but 2 and 3 are already reduced
            if (!var1.sameAs(var2)) {
                vars[e[2]][e[3]] = vars[e[0]][e[1]];
                model.remove(var2);
            }
        }
        model.update();
    }


    private void addValidInequalities(Polygon[] nfps, GRBVar[][] vars) throws GRBException {
        for (int m = 0; m < nfps.length - 1; m++) {
            Polygon p = nfps[m];
            for (int n = m + 1; n < nfps.length; n++) {
                Polygon q = nfps[n];
                for (int k = 0; k < p.size; k++) {
                    Point k1 = p.get(k);
                    Point k2 = p.get(k + 1);
                    for (int e = 0; e < q.size; e++) {
                        Point e1 = q.get(e);
                        Point e2 = q.get(e + 1);
                        if (intersectOutside(k1, k2, e1, e2)) {
                            if (Point.left(k1, k2, e1)) {
                                if (Point.left(e1, e2, k1)) {
                                    // case d from Cherri et al.
                                    // v_k + v_e <= 1
                                    GRBLinExpr caseD = new GRBLinExpr();
                                    caseD.addTerm(1.0, vars[m][k]);
                                    caseD.addTerm(1.0, vars[n][e]);
                                    model.addConstr(caseD, GRB.LESS_EQUAL, 1.0, "");

                                }
                            }
                        }
                    }
                }

//                for (int k = 0; k < p.size; k++) {
//                    Point k1 = p.get(k);
//                    Point k2 = p.get(k + 1);
//                    for (int e = 0; e < q.size; e++) {
//                        Point e1 = q.get(e);
//                        Point e2 = q.get(e + 1);
//                        if (intersectOutside(k1, k2, e1, e2)) {
//                            if (Point.left(k1, k2, e1)) {
//                                if (Point.right(e1, e2, k1)) {
//                                    // v_k <= v_e
//                                    GRBLinExpr expr = new GRBLinExpr();
//                                    expr.addTerm(1.0, vars[m][k]);
//                                    expr.addTerm(-1.0, vars[n][e]);
//                                    expr.addTerm(1.0, i);
//                                    expr.addTerm(1.0, j);
//                                    model.addConstr(expr, GRB.LESS_EQUAL, 2.0, "");
//
//                                } else
//                                if (Point.left(e1, e2, k1)) {
//                                    // v_k + v_e <= 1
//                                    GRBLinExpr expr = new GRBLinExpr();
//                                    expr.addTerm(1.0, vars[m][k]);
//                                    expr.addTerm(1.0, vars[n][e]);
//                                    expr.addTerm(1.0, i);
//                                    expr.addTerm(1.0, j);
//                                    model.addConstr(expr, GRB.LESS_EQUAL, 3.0, "");
//
//                                }
//                            }
//                            else if (Point.right(k1, k2, e1)) {
//                                if (Point.right(e1, e2, k1)) {
//                                    //v_k + v_e >= 1
//                                    GRBLinExpr expr = new GRBLinExpr();
//                                    expr.addTerm(-1.0, vars[m][k]);
//                                    expr.addTerm(-1.0, vars[n][e]);
//                                    expr.addTerm(1.0, i);
//                                    expr.addTerm(1.0, j);
//                                    model.addConstr(expr, GRB.LESS_EQUAL, 1.0, "");
//
//                                } else if (Point.left(e1, e2, k1)) {
//                                    //v_k >= v_e
//                                    GRBLinExpr expr = new GRBLinExpr();
//                                    expr.addTerm(-1.0, vars[m][k]);
//                                    expr.addTerm(1.0, vars[n][e]);
//                                    expr.addTerm(1.0, i);
//                                    expr.addTerm(1.0, j);
//                                    model.addConstr(expr, GRB.LESS_EQUAL, 2.0, "");
//
//                                }
//                            }
//                        }
//                    }
//                }
            }
        }
    }


    /**
     * Checks the condition for the valid inequalities that the edges (k1, k2) and (e1, e2)
     * intersect outside the container (only bounding box)
     */
    private boolean intersectOutside(Point k1, Point k2, Point e1, Point e2) {
        Point intersection = Point.findIntersection(k1, k2, e1, e2);
        Polygon container = instance.container;

        return intersection.x > container.width || intersection.x < -container.width ||
                intersection.y > container.height || intersection.y < -container.height;
    }
}
