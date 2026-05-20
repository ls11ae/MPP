package ContinuousModels;

import DataStructure.ConflictGraph;
import DataStructure.ModelType;
import DataStructure.Point;
import DataStructure.Polygon;
import Main.Main;
import com.gurobi.gurobi.GRB;
import com.gurobi.gurobi.GRBException;
import com.gurobi.gurobi.GRBLinExpr;
import com.gurobi.gurobi.GRBVar;

import java.util.ArrayList;
import java.util.Arrays;

/**
 * This class implements the model based on the NFP-CM-VS by Díaz and Ortuño from
 * Mixed-integer programming models for irregular strip packing based on vertical slices and feasibility cuts.
 */
public class NFP_CM_VS extends ContinuousModel {
    public NFP_CM_VS() throws GRBException {
        super(ModelType.NFP_CM_VS);
    }

    @Override
    void addNonOverlappingConstraints() throws GRBException {
        Polygon[][] convexParts = decomposePolygons();

        // required for the valid inequalities
        Polygon[][][] allNFPs = createAllNFPs(convexParts);
        GRBVar[][][][][][] allVars = createAllVariables(allNFPs);

        // 3.3.1 (eq. 34 in the paper)
        eliminateVariablesForIdenticalPieces(allNFPs, allVars);

        for (int i = 0; i < instance.num_items; i++) {
            Polygon itemI = instance.items[i];
            for (int j = i; j < instance.num_items; j++) {
                Polygon itemJ = instance.items[j];
                if (i == j && itemI.quantity == 1) {
                    continue;
                }

                Polygon[] nfps = allNFPs[i][j];

                for (int k = 0; k < itemI.quantity; k++) {
                    int start = i == j ? k + 1 : 0;
                    for (int l = start; l < itemJ.quantity; l++) {

                        GRBVar[][] vars = allVars[i][j][k][l];

                        // 3.3.2 (1) in the paper
                        removeVariablesForIdenticalFeasibleSubregions(nfps, vars);

                        for (int m = 0; m < nfps.length; m++) {

                            Polygon nfp = nfps[m];

                            addSumConstraints(vars[m], packed[i][k], packed[j][l]);

                            addDisjointDecompositionConstraint(vars[m],
                                    xPos[i][k], xPos[j][l], packed[i][k], packed[j][l], nfp, itemI, itemJ);

                            for (int e = 0; e < nfp.size; e++) {

                                if (vars[m][e] != null) {
                                    addRightOnEdgeConstraint(itemI, itemJ, nfp, e,
                                            vars[m][e], xPos[i][k], yPos[i][k], xPos[j][l], yPos[j][l]);
                                }
                            }
                        }
                        // 3.3.2 (2)
                        addSubsumptionCuts(nfps, vars);

                        // 3.3.2 (3)
                        addNonOverlappingFeasibleRegionsCuts(nfps, vars);
                    }
                }
                // 3.3.1 (eq. 35)
                if (i < j && itemJ.quantity > 1) {
                    addValidInequalitiesForIdenticalPieces(nfps, allVars[i][j], itemI.quantity, itemJ.quantity);
                }

            }
        }
        addValidCuts(convexParts, allNFPs, allVars);
    }


    private void removeVariablesForIdenticalFeasibleSubregions(Polygon[] nfps, GRBVar[][] vars) throws GRBException {

        for (int i = 0; i < nfps.length - 1; i++) {
            for (int j = i + 1; j < nfps.length; j++) {
                if (Math.abs(nfps[i].minX - nfps[j].minX) <= Main.EPSILON) {
                    GRBVar v1 = vars[i][vars[i].length - 2];
                    GRBVar v2 = vars[j][vars[j].length - 2];
                    if (!v1.sameAs(v2)) {
                        model.remove(v2);
                        vars[j][vars[j].length - 2] = v1;
                    }
                }
                if (Math.abs(nfps[i].maxX - nfps[j].maxX) <= Main.EPSILON) {
                    GRBVar v1 = vars[i][vars[i].length - 1];
                    GRBVar v2 = vars[j][vars[j].length - 1];
                    if (!v1.sameAs(v2)) {
                        model.remove(v2);
                        vars[j][vars[j].length - 1] = v1;
                    }
                }
            }
        }
    }

    private void addSubsumptionCuts(Polygon[] nfps, GRBVar[][] vars) throws GRBException {
        for (int i = 0; i < nfps.length - 1; i++) {
            for (int j = i + 1; j < nfps.length; j++) {

                addSubsumptionCuts(nfps[i], nfps[j], vars[i], vars[j]);
                addSubsumptionCuts(nfps[j], nfps[i], vars[j], vars[i]);
            }
        }

    }

    private void addSubsumptionCuts(Polygon p1, Polygon p2, GRBVar[] v1, GRBVar[] v2) throws GRBException {
        for (int i = 0; i < p1.size; i++) {
            Point e1 = p1.get(i);
            Point e2 = p1.get(i + 1);
            // not correct implemented: side edges can also completely cover other areas
            if (isSideEdge(e1, e2) || v1[i] == null) {
                continue;
            }
            boolean isBottomEdge = e1.x < e2.x;
            GRBLinExpr expr = new GRBLinExpr();
            for (int j = 0; j < p2.size; j++) {
                Point k1 = p2.get(j);
                Point k2 = p2.get(j + 1);

                if (isSideEdge(k1, k2) || v2[j] == null) {
                    continue;
                }
                if (isBottomEdge) {
                    if (k1.x < k2.x && e1.x <= k1.x && e2.x >= k2.x) {
                        if ((Point.right(e1, e2, k1) || p1.contains(k1, false)) &&
                                (Point.right(e1, e2, k2) || p1.contains(k2, false))) {
                            expr.addTerm(1.0, v2[j]);
                        }
                    }
                } else {
                    if (k1.x > k2.x && e1.x >= k1.x && e2.x <= k2.x) {
                        if ((Point.right(e1, e2, k1) || p1.contains(k1, false)) &&
                                (Point.right(e1, e2, k2) || p1.contains(k2, false))) {
                            expr.addTerm(1.0, v2[j]);
                        }
                    }
                }
            }
            if (expr.size() > 0) {
                expr.addTerm(-1.0, v1[i]);
                model.addConstr(expr, GRB.LESS_EQUAL, 0.0, "");
            }
        }
    }

    private void eliminateVariablesForIdenticalPieces(
            Polygon[][][] allNFPs, GRBVar[][][][][][] allVars) throws GRBException {

        for (int i = 0; i < instance.num_items; i++) {
            Polygon item = instance.items[i];
            if (item.quantity > 1) {
                Polygon[] nfps = allNFPs[i][i];
                for (int j = 0; j < nfps.length; j++) {
                    Polygon nfp = nfps[j];
                    for (int e = 0; e < nfp.size; e++) {
                        Point e1 = nfp.get(e);
                        Point e2 = nfp.get(e + 1);
                        // is negative bottom edge
                        if (!isSideEdge(e1, e2) && e1.x < e2.x && e1.y <= 0.0 && e2.y <= 0.0) {
                            if (e1.y == 0.0 && e2.y == 0.0) {
                                continue;
                            }
                            for (int k = 0; k < item.quantity - 1; k++) {
                                for (int l = k + 1; l < item.quantity; l++) {
                                    model.remove(allVars[i][i][k][l][j][e]);
                                    allVars[i][i][k][l][j][e] = null;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    private void addValidInequalitiesForIdenticalPieces(
            Polygon[] nfps, GRBVar[][][][] vars, int quantityI, int quantityJ) throws GRBException {

        for (int i = 0; i < nfps.length; i++) {
            Polygon nfp = nfps[i];
            ArrayList<Integer> botEdges = new ArrayList<>();
            ArrayList<Integer> topEdges = new ArrayList<>();
            for (int e = 0; e < nfp.size; e++) {
                Point e1 = nfp.get(e);
                Point e2 = nfp.get(e + 1);
                if (isSideEdge(e1, e2)) {
                    continue;
                } else if (e1.x < e2.x) {
                    botEdges.add(e);
                } else {
                    topEdges.add(e);
                }
            }
            for (int k = 0; k < quantityI; k++) {
                for (int l = 0; l < quantityJ - 1; l++) {
                    for (int m = l + 1; m < quantityJ; m++) {
                        GRBLinExpr expr = new GRBLinExpr();
                        for (int j : botEdges) {
                            if (vars[k][m][i][j] != null) {
                                expr.addTerm(1.0, vars[k][m][i][j]);
                            }
                        }
                        for (int j : topEdges) {
                            if (vars[k][l][i][j] != null) {
                                expr.addTerm(1.0, vars[k][l][i][j]);
                            }
                        }
                        model.addConstr(expr, GRB.LESS_EQUAL, 1.0, "");
                    }
                }
            }
        }
    }


    private void addNonOverlappingFeasibleRegionsCuts(Polygon[] nfps, GRBVar[][] vars) throws GRBException {
        ConflictGraph graph = new ConflictGraph();
        int[][] nodeIDs = createNodeIDs(vars);
        for (int i = 0; i < nfps.length - 1; i++) {
            Polygon nfpI = nfps[i];
            int leftI = vars[i].length - 2;
            int rightI = vars[i].length - 1;

            for (int j = i + 1; j < nfps.length; j++) {
                Polygon nfpJ = nfps[j];
                int leftJ = vars[j].length - 2;
                int rightJ = vars[j].length - 1;

                //Left Edge I
                int idI = nodeIDs[i][leftI];
                GRBVar varI = vars[i][leftI];
                if (nfpJ.maxX >= nfpI.minX) {
                    graph.addEdge(idI, nodeIDs[j][rightJ], varI, vars[j][rightJ]);
                }
                for (int k = 0; k < nfpJ.size; k++) {
                    Point k1 = nfpJ.get(k);
                    Point k2 = nfpJ.get(k + 1);
                    if (isSideEdge(k1, k2) || vars[j][k] == null) continue;
                    if (Math.min(k1.x, k2.x) >= nfpI.minX) {
                        graph.addEdge(idI, nodeIDs[j][k], varI, vars[j][k]);
                    }
                }

                //Right Edge I
                idI = nodeIDs[i][rightI];
                varI = vars[i][rightI];
                if (nfpJ.minX <= nfpI.maxX) {
                    graph.addEdge(idI, nodeIDs[j][leftJ], varI, vars[j][leftJ]);
                }
                for (int k = 0; k < nfpJ.size; k++) {
                    Point k1 = nfpJ.get(k);
                    Point k2 = nfpJ.get(k + 1);
                    if (isSideEdge(k1, k2) || vars[j][k] == null) continue;
                    if (Math.max(k1.x, k2.x) <= nfpI.maxX) {
                        graph.addEdge(idI, nodeIDs[j][k], varI, vars[j][k]);
                    }
                }

                for (int e = 0; e < nfpI.size; e++) {
                    Point e1 = nfpI.get(e);
                    Point e2 = nfpI.get(e + 1);
                    if (isSideEdge(e1, e2) || vars[i][e] == null) continue;
                    boolean isBottomEdgeI = e1.x < e2.x;
                    idI = nodeIDs[i][e];
                    varI = vars[i][e];
                    if (isBottomEdgeI) {
                        // bottom edge I and left edge J
                        if (e1.x >= nfpJ.minX) {
                            graph.addEdge(idI, nodeIDs[j][leftJ], varI, vars[j][leftJ]);
                        }
                        // bottom edge I and right edge J
                        if (e2.x <= nfpJ.maxX) {
                            graph.addEdge(idI, nodeIDs[j][rightJ], varI, vars[j][rightJ]);
                        }

                        for (int k = 0; k < nfpJ.size; k++) {
                            Point k1 = nfpJ.get(k);
                            Point k2 = nfpJ.get(k + 1);
                            if (isSideEdge(k1, k2) || vars[j][k] == null) continue;

                            boolean isBottomEdgeJ = k1.x < k2.x;
                            if ((isBottomEdgeJ && (e1.x >= k2.x || e2.x <= k1.x)) ||
                                    (!isBottomEdgeJ && (e1.x >= k1.x || e2.x <= k2.x ||
                                            Math.min(k1.y, k2.y) >= Math.max(e1.y, e2.y)))) {
                                graph.addEdge(idI, nodeIDs[j][k], varI, vars[j][k]);
                            }
                        }
                    } else {
                        // top edge I and left edge J
                        if (e2.x >= nfpJ.minX) {
                            graph.addEdge(idI, nodeIDs[j][leftJ], varI, vars[j][leftJ]);
                        }
                        // top edge I and right edge J
                        if (e1.x <= nfpJ.maxX) {
                            graph.addEdge(idI, nodeIDs[j][rightJ], varI, vars[j][rightJ]);
                        }

                        for (int k = 0; k < nfpJ.size; k++) {
                            Point k1 = nfpJ.get(k);
                            Point k2 = nfpJ.get(k + 1);
                            if (isSideEdge(k1, k2) || vars[j][k] == null) continue;

                            boolean isBottomEdgeJ = k1.x < k2.x;
                            if ((isBottomEdgeJ && (e2.x >= k2.x || e1.x <= k1.x ||
                                    Math.min(e1.y, e2.y) >= Math.max(k1.y, k2.y))) ||
                                    (!isBottomEdgeJ && (e2.x >= k1.x || e1.x <= k2.x))) {
                                graph.addEdge(idI, nodeIDs[j][k], varI, vars[j][k]);
                            }
                        }
                    }
                }
            }
        }

        GRBVar[][] cuts = graph.getCliqueCover();
        for (GRBVar[] cut : cuts) {
            GRBLinExpr expr = new GRBLinExpr();
            double[] coe = new double[cut.length];
            Arrays.fill(coe, 1.0);
            expr.addTerms(coe, cut);
            model.addConstr(expr, GRB.LESS_EQUAL, 1.0, "");

        }
    }

    private int[][] createNodeIDs(GRBVar[][] vars) {
        int id = 0;
        int[][] nodeIDs = new int[vars.length][];
        for (int i = 0; i < vars.length; i++) {
            nodeIDs[i] = new int[vars[i].length];
            for (int j = 0; j < vars[i].length; j++) {
                nodeIDs[i][j] = id++;
            }
        }
        return nodeIDs;
    }


    private Polygon[][][] createAllNFPs(Polygon[][] convexParts) {
        Polygon[][][] nfps = new Polygon[instance.num_items][instance.num_items][];
        for (int i = 0; i < instance.num_items; i++) {
            Polygon itemI = instance.items[i];
            for (int j = i; j < instance.num_items; j++) {
                if (i == j && itemI.quantity == 1) {
                    continue;
                }
                nfps[i][j] = createNFPs(convexParts[i], convexParts[j]);
            }
        }

        return nfps;
    }

    private GRBVar[][][][][][] createAllVariables(Polygon[][][] nfps) throws GRBException {
        GRBVar[][][][][][] vars = new GRBVar[instance.num_items][instance.num_items][][][][];
        for (int i = 0; i < instance.num_items; i++) {
            Polygon itemI = instance.items[i];
            for (int j = i; j < instance.num_items; j++) {
                Polygon itemJ = instance.items[j];
                if (i == j && itemI.quantity == 1) {
                    continue;
                }
                vars[i][j] = new GRBVar[itemI.quantity][][][];
                for (int k = 0; k < itemI.quantity; k++) {
                    int start = i == j ? k + 1 : 0;
                    vars[i][j][k] = new GRBVar[itemJ.quantity][][];
                    for (int l = start; l < itemJ.quantity; l++) {
                        vars[i][j][k][l] = createEdgeVariables(nfps[i][j]);
                    }
                }
            }
        }

        return vars;
    }


    /**
     * This methode is copied from Díaz and Ortuño.
     */
    private void addValidCuts(Polygon[][] convexParts, Polygon[][][] nfps, GRBVar[][][][][][] vars) throws GRBException {
        double containerWidth = instance.container.width;
        int n = instance.num_items;
        for (int i = 0; i < n; i++) {
            for (int iQ = 0; iQ < instance.items[i].quantity; iQ++) {
                for (int j = i; j < n; j++) {
                    for (int jQ = i == j ? iQ + 1 : 0; jQ < instance.items[j].quantity; jQ++) {


                        double li_max = instance.items[i].width;
                        double lj_max = instance.items[j].width;

                        DoubleInterval[] feasibleRegions = new DoubleInterval[3];

                        for (int f = 0; f < convexParts[i].length; f++) {
                            for (int g = 0; g < convexParts[j].length; g++) {

                                Polygon NFPfg = nfps[i][j][f * convexParts[j].length + g];

                                for (int k = 0; k < NFPfg.size + 2; k++) {

                                    feasibleRegions[0] = null;

                                    if (k == NFPfg.size) {
                                        feasibleRegions[0] = new DoubleInterval(-containerWidth + li_max, NFPfg.minX);
                                    } else if (k == NFPfg.size + 1) {
                                        feasibleRegions[0] = new DoubleInterval(NFPfg.maxX, containerWidth - lj_max);
                                    } else if (!isSideEdge(NFPfg.get(k), NFPfg.get(k + 1)) &&
                                            vars[i][j][iQ][jQ][f * convexParts[j].length + g][k] != null) {
                                        Point a = NFPfg.get(k);
                                        Point b = NFPfg.get(k + 1);

                                        feasibleRegions[0] = new DoubleInterval(Math.min(a.x, b.x), Math.max(a.x, b.x));
                                    } else {
                                        continue;
                                    }


                                    for (int u = j; u < n; u++) {
                                        for (int uQ = u == j ? jQ + 1 : 0; uQ < instance.items[u].quantity; uQ++) {

                                            double lu_max = instance.items[u].width;

                                            for (int h = 0; h < convexParts[u].length; h++) {

                                                Polygon NFPfh = nfps[i][u][f * convexParts[u].length + h];
                                                for (int k1 = 0; k1 < NFPfh.size + 2; k1++) {

                                                    feasibleRegions[1] = null;

                                                    if (k1 == NFPfh.size) {
                                                        feasibleRegions[1] = new DoubleInterval(-containerWidth + li_max, NFPfh.minX);
                                                    } else if (k1 == NFPfh.size + 1) {
                                                        feasibleRegions[1] = new DoubleInterval(NFPfh.maxX, containerWidth - lu_max);
                                                    } else if (!isSideEdge(NFPfh.get(k1), NFPfh.get(k1 + 1)) &&
                                                            vars[i][u][iQ][uQ][f * convexParts[u].length + h][k1] != null) {
                                                        Point a = NFPfh.get(k1);
                                                        Point b = NFPfh.get(k1 + 1);
                                                        feasibleRegions[1] = new DoubleInterval(Math.min(a.x, b.x), Math.max(a.x, b.x));
                                                    } else {
                                                        continue;
                                                    }

                                                    GRBLinExpr expr = new GRBLinExpr();
                                                    Polygon NFPgh = nfps[j][u][g * convexParts[u].length + h];

                                                    for (int k2 = 0; k2 < NFPgh.size + 2; k2++) {
                                                        feasibleRegions[2] = null;

                                                        if (k2 == NFPgh.size) {
                                                            feasibleRegions[2] = new DoubleInterval(-containerWidth + lj_max, NFPgh.minX);
                                                        } else if (k2 == NFPgh.size + 1) {
                                                            feasibleRegions[2] = new DoubleInterval(NFPgh.maxX, containerWidth - lu_max);
                                                        } else if (!isSideEdge(NFPgh.get(k2), NFPgh.get(k2 + 1)) &&
                                                                vars[j][u][jQ][uQ][g * convexParts[u].length + h][k2] != null) {
                                                            Point a = NFPgh.get(k2);
                                                            Point b = NFPgh.get(k2 + 1);
                                                            feasibleRegions[2] = new DoubleInterval(Math.min(a.x, b.x), Math.max(a.x, b.x));
                                                        } else {
                                                            continue;
                                                        }
                                                        DoubleInterval relativeFeasibleRegion = feasibleRegions[0].add(feasibleRegions[2]);

                                                        if (relativeFeasibleRegion.intersection(feasibleRegions[1]) == null) {
                                                            expr.addTerm(1.0, vars[j][u][jQ][uQ][g * convexParts[u].length + h][k2]);
                                                        }
                                                    }

                                                    if (expr.size() > 0) {
                                                        expr.addTerm(1.0, vars[i][j][iQ][jQ][f * convexParts[j].length + g][k]);
                                                        expr.addTerm(1.0, vars[i][u][iQ][uQ][f * convexParts[u].length + h][k1]);

                                                        model.addConstr(expr, GRB.LESS_EQUAL, 2.0, "").set(GRB.IntAttr.Lazy, -1);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    private static class DoubleInterval {
        double start, end;

        public DoubleInterval(double start, double end) {
            this.start = start;
            this.end = end;
        }

        /**
         * This function sums other interval to the current interval
         *
         * @param otherInterval
         * @return
         */

        public DoubleInterval add(
                DoubleInterval otherInterval) {
            return (new DoubleInterval(start + otherInterval.start,
                    end + otherInterval.end));
        }

        public DoubleInterval intersection(
                DoubleInterval otherInterval) {
            // We initialize the output

            DoubleInterval result = null;

            // We compute the intersection interval

            if ((otherInterval.end >= start)
                    && (end >= otherInterval.start)) {
                result = new DoubleInterval(Math.max(start, otherInterval.start),
                        Math.min(end, otherInterval.end));
            }

            // We return the result

            return (result);
        }
    }
}
