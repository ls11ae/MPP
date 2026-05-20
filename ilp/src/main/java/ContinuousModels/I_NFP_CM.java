package ContinuousModels;

import DataStructure.ModelType;
import DataStructure.Polygon;
import com.gurobi.gurobi.GRBException;
import com.gurobi.gurobi.GRBVar;

/**
 * This class implements the model based on the improved NFP-CM by Rodrigues et al.
 * in MIP models for the irregular strip packing problem: new symmetry breaking constraints.
 * <a href="https://doi.org/10.1051/itmconf/20171400005">DOI</a>
 */
public class I_NFP_CM extends ContinuousModel {
    public I_NFP_CM() throws GRBException {
        super(ModelType.I_NFP_CM);
    }

    @Override
    void addNonOverlappingConstraints() throws GRBException {

        Polygon[][] convexParts = decomposePolygons();

        for (int i = 0; i < instance.num_items; i++) {
            Polygon itemI = instance.items[i];
            for (int j = i; j < instance.num_items; j++) {
                Polygon itemJ = instance.items[j];
                if (i == j && itemI.quantity == 1) {
                    continue;
                }

                Polygon[] nfps = createNFPs(convexParts[i], convexParts[j]);

                for (int k = 0; k < itemI.quantity; k++) {
                    int start = i == j ? k + 1 : 0;
                    for (int l = start; l < itemJ.quantity; l++) {

                        GRBVar[][] vars = createEdgeVariables(nfps);

                        for (int m = 0; m < nfps.length; m++) {

                            Polygon nfp = nfps[m];
                            GRBVar x_i = xPos[i][k];
                            GRBVar y_i = yPos[i][k];
                            GRBVar x_j = xPos[j][l];
                            GRBVar y_j = yPos[j][l];

                            // equation (6)
                            addSumConstraints(vars[m], packed[i][k], packed[j][l]);

                            for (int e = 0; e < nfp.size; e++) {
                                GRBVar v = vars[m][e];

                                // left previous edge (eq. (8))
                                addLeftOnEdgeConstraint(itemI, itemJ, nfp, e - 1, v, x_i, y_i, x_j, y_j);
                                // right current edge (eq. (5))
                                addRightOnEdgeConstraint(itemI, itemJ, nfp, e, v, x_i, y_i, x_j, y_j);

                            }
                        }
                    }
                }
            }
        }
    }
}
