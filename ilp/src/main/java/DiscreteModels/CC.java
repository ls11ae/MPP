package DiscreteModels;

import DataStructure.*;
import Geometry.InnerFitPolygon;
import Geometry.NoFitPolygon;
import com.gurobi.gurobi.GRB;
import com.gurobi.gurobi.GRBException;
import com.gurobi.gurobi.GRBLinExpr;
import com.gurobi.gurobi.GRBVar;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;

/**
 * This class implements the model based on the Clique-Cover-Model by Rodrigues and Toledo from
 * A clique covering MIP model for the irregular strip packing problem.
 */
public class CC extends Model {

    private Instance instance;
    private GRBVar[][][] variables;
    ConflictGraph graph;

    public CC() throws GRBException {
        super(ModelType.CC);
    }

    @Override
    public void createModel(Instance instance, boolean writeModelToFile) throws GRBException {
        this.instance = instance;
        graph = new ConflictGraph();
        createInnerFitVariables();
        addNonOverlappingConstraints();
        if (writeModelToFile) {
            String path = "./Results/Files/Models/" + instance.type;
            new File(path).mkdirs();
            model.write(path + "/" + instance.instance_name + "_CC.mps");
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
     * The grid resolution is always 1, scale the instance to change the number of dots.
     */
    private void createInnerFitVariables() throws GRBException {

        variables = new GRBVar[instance.num_items][][];

        GRBLinExpr area = new GRBLinExpr();
        GRBLinExpr objective = new GRBLinExpr();
        Polygon container = instance.container;
        if (container.width > 75.0 || container.height > 75) {
            System.out.println("Container too big > 10 million constraints");
            System.exit(-1);
        }

        for (int i = 0; i < instance.num_items; i++) {
            Polygon item = instance.items[i];
            Polygon innerFitPolygon = InnerFitPolygon.create(container, item);
            if (innerFitPolygon == null) {
                System.out.println("IFP is null, robustness problem with clipper");
                System.exit(-1);
            }

            innerFitPolygon.setWidthAndHeight();
            variables[i] = new GRBVar[(int) innerFitPolygon.height + 1][(int) innerFitPolygon.width + 1];


            GRBLinExpr expr = new GRBLinExpr();
            for (int j = 0; j < variables[i].length; j++) {
                for (int k = 0; k <= variables[i][j].length; k++) {
                    // inefficient, better with scanline
                    if (innerFitPolygon.contains(new Point(k, j), false)) {
                        GRBVar var = model.addVar(0.0, 1.0,
                                item.value, GRB.BINARY, i + "_" + j + "_" + k);
                        variables[i][j][k] = var;
                        expr.addTerm(1.0, var);
                        area.addTerm(item.area, var);
                        objective.addTerm(item.value, var);

                    }
                }
            }
            model.addConstr(expr, GRB.LESS_EQUAL, item.quantity, "");
        }
        model.addConstr(area, GRB.LESS_EQUAL, container.area, "");
        model.setObjective(objective, GRB.MAXIMIZE);
    }

    /**
     *
     */
    private void addNonOverlappingConstraints() throws GRBException {
        for (int i = 0; i < instance.num_items; i++) {
            Polygon itemI = instance.items[i];
            int skip = itemI.quantity > 1 ? 0 : 1;
            for (int j = i + skip; j < instance.num_items; j++) {
                Polygon itemJ = instance.items[j];
                Polygon[] nfpWithHoles = NoFitPolygon.createNonConvex(itemI, itemJ);
                Polygon nfp = nfpWithHoles[0];
                ArrayList<Integer> xs = new ArrayList<>();
                ArrayList<Integer> ys = new ArrayList<>();
                double minX = Double.MAX_VALUE;
                double maxX = -Double.MAX_VALUE;
                double minY = Double.MAX_VALUE;
                double maxY = -Double.MAX_VALUE;
                for (Point point : nfp.points) {
                    minX = Math.min(minX, point.x);
                    maxX = Math.max(maxX, point.x);
                    minY = Math.min(minY, point.y);
                    maxY = Math.max(maxY, point.y);
                }
                for (int k = (int) minY; k < maxY; k++) {
                    for (int l = (int) minX; l < maxX; l++) {
                        Point p = new Point(l, k);
                        if (nfp.contains(p, true)) {
                            boolean insideHole = false;
                            for (int m = 1; m < nfpWithHoles.length; m++) {
                                if (nfpWithHoles[m].contains(p, false)) {
                                    insideHole = true;
                                    break;
                                }
                            }
                            if (!insideHole) {
                                xs.add(l);
                                ys.add(k);
                            }
                        }
                    }
                }

                for (int k = 0; k < variables[i].length; k++) {
                    for (int l = 0; l < variables[i][0].length; l++) {
                        GRBVar var = variables[i][k][l];
                        if (var != null) {
                            for (int m = 0; m < xs.size(); m++) {
                                int x = l + xs.get(m);
                                int y = k + ys.get(m);
                                GRBVar var2 = get(j, x, y);
                                if (var2 != null) {
                                    if (var == var2) {
                                        continue;
                                    }
                                    // unique ids, i,k,l < 100
                                    graph.addEdge(i + k * 100 + l * 10000, j + y * 100 + x * 10000, var, var2);
                                }
                            }
                        }
                    }
                }
            }
        }

        System.out.println("Run ECC8 on conflict graph with " + graph.getSize() + " edges.");
        if (graph.getSize() > 1000000) {
            System.out.println("ECC8 might not terminate!");
        }
        // adds the edge-clique-cover inequalities from the conflict graph
        for (GRBVar[] cut : graph.getCliqueCover()) {
            GRBLinExpr expr = new GRBLinExpr();
            double[] coe = new double[cut.length];
            Arrays.fill(coe, 1.0);
            expr.addTerms(coe, cut);
            model.addConstr(expr, GRB.LESS_EQUAL, 1.0, "");
        }
    }

    private GRBVar get(int i, int x, int y) {
        if (x < 0 || y < 0 || y >= variables[i].length || x >= variables[i][0].length) {
            return null;
        } else {
            return variables[i][y][x];
        }
    }

    Solution getSolution() throws GRBException {
        int total_value = 0;
        int num_included_items = 0;
        ArrayList<Integer> item_indices = new ArrayList<>();
        ArrayList<Double> x_translation = new ArrayList<>();
        ArrayList<Double> y_translation = new ArrayList<>();
        for (int i = 0; i < instance.num_items; i++) {
            Polygon item = instance.items[i];
            for (int k = 0; k < variables[i].length; k++) {
                for (int l = 0; l < variables[i][0].length; l++) {
                    GRBVar var = variables[i][k][l];
                    if (var != null && var.get(GRB.DoubleAttr.X) > 0.5) {
                        total_value += item.value;
                        num_included_items++;
                        item_indices.add((item.index));
                        x_translation.add(l - item.xTranslation - instance.container.xTranslation);
                        y_translation.add(k - item.yTranslation - instance.container.yTranslation);
                    }
                }
            }
        }

        return new Solution(instance.type, instance.instance_name, type, total_value, num_included_items,
                item_indices.stream().mapToInt(x -> x).toArray(), x_translation.stream().mapToDouble(x -> x).toArray(),
                y_translation.stream().mapToDouble(x -> x).toArray());
    }
}
