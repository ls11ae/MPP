package ContinuousModels;

import DataStructure.ModelType;
import DataStructure.Polygon;
import Geometry.ConvexDecomposition;
import Geometry.GrahamScan;
import Geometry.InnerFitPolygon;
import Geometry.NoFitPolygon;
import com.gurobi.gurobi.GRB;
import com.gurobi.gurobi.GRBException;
import com.gurobi.gurobi.GRBVar;

import java.util.ArrayList;
import java.util.Arrays;

/**
 * This class implements the model based on the Horizontal-Slices-Model by Alvarez-Valdes and Tamarit from
 * A branch & bound algorithm for cutting and packing irregularly shaped pieces.
 */
public class HS2 extends ContinuousModel {
    public HS2() throws GRBException {
        super(ModelType.HS2);
    }

    @Override
    void addNonOverlappingConstraints() throws GRBException {

        for (int i = 0; i < instance.num_items; i++) {
            Polygon itemI = instance.items[i];
            for (int j = i; j < instance.num_items; j++) {
                Polygon itemJ = instance.items[j];
                if (i == j && itemI.quantity == 1) {
                    continue;
                }
                Polygon[] nfpParts = NoFitPolygon.createNonConvex(instance.items[i], instance.items[j]);
                Polygon nfp = nfpParts[0];
                nfp.removeCollinearPoints();

                Polygon convexHull = GrahamScan.getConvexHull(Arrays.asList(nfp.points));
                convexHull.setMinMaxX();
                int left = convexHull.size;
                int right = left + 1;

                Polygon[] innerPolygons = InnerFitPolygon.createInnerPolygons(nfp);
                Polygon[] holes = getHoles(nfpParts);



                for (int k = 0; k < itemI.quantity; k++) {
                    int start = i == j ? k + 1 : 0;
                    for (int l = start; l < itemJ.quantity; l++) {


                        // creates all binary variables for edges, inner convex parts and holes
                        // (can't deal with special cases)
                        GRBVar[] vars = new GRBVar[convexHull.size + innerPolygons.length + holes.length + 2];
                        for (int e = 0; e < convexHull.size; e++) {
                            if (!isSideEdge(convexHull.get(e), convexHull.get(e + 1))) {
                                vars[e] = model.addVar(0.0, 1.0, 0.0, GRB.BINARY, "");
                            }
                        }
                        vars[left] = model.addVar(0.0, 1.0, 0.0, GRB.BINARY, "");
                        vars[right] = model.addVar(0.0, 1.0, 0.0, GRB.BINARY, "");

                        for (int n = 0; n < innerPolygons.length; n++) {
                            vars[convexHull.size + n + 2] =
                                    model.addVar(0.0, 1.0, 0.0, GRB.BINARY, "");
                        }
                        for (int n = 0; n < holes.length; n++) {
                            vars[convexHull.size + innerPolygons.length + n + 2] =
                                    model.addVar(0.0, 1.0, 0.0, GRB.BINARY, "");
                        }

                        GRBVar x_i = xPos[i][k];
                        GRBVar y_i = yPos[i][k];
                        GRBVar x_j = xPos[j][l];
                        GRBVar y_j = yPos[j][l];

                        addSumConstraints(vars, packed[i][k], packed[j][l]);

                        addDisjointDecompositionConstraint(vars, xPos[i][k], xPos[j][l], packed[i][k], packed[j][l],
                                convexHull, innerPolygons, holes, itemI, itemJ);


                        for (int e = 0; e < convexHull.size; e++) {
                            GRBVar v = vars[e];

                            // right current edge
                            if (!isSideEdge(convexHull.get(e), convexHull.get(e + 1))) {

                                addRightOnEdgeConstraint(itemI, itemJ, convexHull, e, v, x_i, y_i, x_j, y_j);
                            }
                        }


                        for (int m = 0; m < innerPolygons.length; m++) {
                            GRBVar v = vars[convexHull.size + 2 + m];
                            Polygon innerPolygon = innerPolygons[m];
                            for (int e = 0; e < innerPolygon.size; e++) {

                                addLeftOnEdgeConstraint(itemI, itemJ, innerPolygon, e, v, x_i, y_i, x_j, y_j);
                            }
                        }
                        for (int m = 0; m < holes.length - 1; m++) {
                            GRBVar v = vars[convexHull.size + innerPolygons.length + 2 + m];
                            Polygon hole = holes[m];
                            for (int e = 0; e < hole.size; e++) {

                                addLeftOnEdgeConstraint(itemI, itemJ, hole, e, v, x_i, y_i, x_j, y_j);
                            }
                        }
                    }
                }
            }
        }
    }



    private Polygon[] getHoles(Polygon[] nfp) {
        ArrayList<Polygon> convexHoles = new ArrayList<>();
        for (int i = 1; i < nfp.length; i++) {
            Polygon hole = nfp[i];
            hole.changeOrientationToCCW(false);
            convexHoles.addAll(Arrays.asList(ConvexDecomposition.decomposePoly(hole)));
        }
        convexHoles.forEach(Polygon::setMinMaxX);
        return convexHoles.toArray(new Polygon[0]);
    }
}
