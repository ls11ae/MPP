package ContinuousModels;

import DataStructure.*;
import Main.Main;
import com.gurobi.gurobi.*;

import java.io.File;
import java.io.IOException;
import java.util.Arrays;
import java.util.logging.*;


/**
 * In this version both models are strictly seperated. The main model calculates the optimal solution (subset)
 * under the current knapsack inequalities and the area cut. So the minimum number of subsets is checked.
 * But the drawback is that the main model needs a few seconds to find the next subset if the instance has more than
 * 24 items.
 */
public class SplitModel2 {
    ContinuousModel support;
    GRBModel main;
    GRBVar[][] packed;
    double bestSolution = 0.0;
    Logger logger;

    public SplitModel2(ModelType basisModel) throws GRBException {
        support = (ContinuousModel) Main.selectModel(basisModel);
        main = new GRBModel(support.env);
        logger = Logger.getAnonymousLogger();
//        logger.setUseParentHandlers(false);
    }

    public void createModel(Instance instance) throws GRBException {

        packed = new GRBVar[instance.num_items][];
        GRBLinExpr objective = new GRBLinExpr();
        GRBLinExpr areaCut = new GRBLinExpr();
        for (int i = 0; i < instance.num_items; i++) {
            Polygon item = instance.items[i];
            packed[i] = new GRBVar[item.quantity];
            for (int j = 0; j < item.quantity; j++) {
                GRBVar v = main.addVar(0.0, 1.0, 0.0, GRB.BINARY, "p_" + i + "_" + j);
                packed[i][j] = v;
                objective.addTerm(item.value, v);
                areaCut.addTerm(item.area, v);

            }
        }
        GRBLinExpr expr;
        for (int i = 0; i < instance.num_items; i++) {
            Polygon item = instance.items[i];
            for (int j = 0; j < item.quantity - 1; j++) {
                // p_i_j >= p_i_j+1
                expr = new GRBLinExpr();
                expr.addTerm(-1.0, packed[i][j]);
                expr.addTerm(1.0, packed[i][j + 1]);
                main.addConstr(expr, GRB.LESS_EQUAL, 0.0, "");
            }
        }
        main.addConstr(areaCut, GRB.LESS_EQUAL, instance.container.area, "");
        main.setObjective(objective, GRB.MAXIMIZE);

        support.createModel(instance, false);
    }

    public Solution solveModel(long timeLimitInSeconds) throws GRBException {
        FileHandler handler = null;
        Polygon[] items = support.instance.items;
        boolean withTimeLimit = timeLimitInSeconds > 0;
        try {
            String path = "./Results/Files/Logs/" + support.instance.type;
            new File(path).mkdirs();
            handler = new FileHandler(path + "/" + support.instance.instance_name + "_split.log");
            SimpleFormatter formatter = new SimpleFormatter();
            handler.setFormatter(formatter);
            logger.addHandler(handler);

        } catch (IOException e) {
            System.out.println("Problem with logger: " + e.getMessage());
            System.exit(-1);
        }
        main.set(GRB.IntParam.LogToConsole, 0);
        support.model.set(GRB.IntParam.LogToConsole, 0);
        main.set(GRB.DoubleParam.MIPGap, 0);
        main.update();
        Solution solution = null;
        logger.info("Instance type: " + support.instance.type +
                "\nInstance name: " + support.instance.instance_name +
                "\nModel type: " + support.type +
                "\nTime limit (secs): " + timeLimitInSeconds + "\n\n");

        int cutCounter = 0;
        double runtimeInMain = 0.0;
        boolean foundOptimalSolution = false;
        long start = System.currentTimeMillis();
        int numOfItems = 0;
        for (Polygon item : items) {
            numOfItems += item.quantity;
        }
        int[] cutLengthCounter = new int[numOfItems];
        while (!foundOptimalSolution) {

            // find and get next subset
            main.optimize();
            runtimeInMain += main.get(GRB.DoubleAttr.Runtime);
            StringBuilder mainItemsAsString = new StringBuilder();
            boolean[][] in = new boolean[packed.length][];
            int numItems = 0;
            for (int i = 0; i < packed.length; i++) {
                in[i] = new boolean[packed[i].length];
                for (int j = 0; j < in[i].length; j++) {
                    in[i][j] = packed[i][j].get(GRB.DoubleAttr.X) > 0.5;
                    support.packed[i][j].set(GRB.DoubleAttr.UB, 0.0);
                    if (in[i][j]) {
                        mainItemsAsString.append("p_").append(items[i].index).append('_').append(j).append(" ");
                        numItems++;
                    }
                }
            }
            logger.info("[" + ((System.currentTimeMillis() - start) / 1000) +
                    "]\tMain solution with value " + main.get(GRB.DoubleAttr.ObjVal) +" in " +
                    String.format("%.3f", main.get(GRB.DoubleAttr.Runtime)) + " secs\nTesting "
                    + numItems + " items: \t\t" + mainItemsAsString);

            // support tests if the subset is a cover
            GRBLinExpr objective = new GRBLinExpr();
            GRBLinExpr coverCut = new GRBLinExpr();
            int subsetSize = 0;
            double value = 0;
            boolean stop = false;
            StringBuilder supportItemsAsString = new StringBuilder();
            // add the items one after the other
            for (int i = 0; i < in.length; i++) {
                if (stop) break;
                for (int j = 0; j < in[i].length; j++) {
                    if (stop) break;
                    if (in[i][j]) {
                        subsetSize++;
                        supportItemsAsString.append("p_").append(items[i].index).append('_').append(j).append(" ");
                        value += support.instance.items[i].value;
                        objective.addTerm(1.0, support.packed[i][j]);
                        coverCut.addTerm(1.0, packed[i][j]);
                        support.packed[i][j].set(GRB.DoubleAttr.UB, 1.0);
                        support.model.setObjective(objective, GRB.MAXIMIZE);
                        if (withTimeLimit) {
                            support.model.set(GRB.DoubleParam.TimeLimit,
                                    Math.max(0, timeLimitInSeconds - ((System.currentTimeMillis() - start) / 1000)));
                        }
                        support.model.optimize();
                        if (withTimeLimit && support.model.get(GRB.IntAttr.Status) == GRB.TIME_LIMIT) {
                            logger.info("Time limit reached!\nBest feasible solution has value " + bestSolution);
                            handler.close();
                            return solution;
                        }
                        // found a cover: add knapsack inequality into the main model
                        if (support.model.get(GRB.DoubleAttr.ObjVal) < subsetSize) {
                            cutLengthCounter[coverCut.size()]++;
                            supportItemsAsString.append("\n");
                            logger.info("Added cover cut (" + (++cutCounter) + "):\t" + supportItemsAsString);
                            main.addConstr(coverCut, GRB.LESS_EQUAL, coverCut.size() - 1, "");
                            main.update();
                            stop = true;
                        } else {
                            if (value > bestSolution) {
                                bestSolution = value;
                                logger.info("Better feasible solution with value "
                                        + value + "\nItems:\t\t\t\t\t" + supportItemsAsString);
                                solution = support.getSolution();
                            }
                        }
                    }
                }
            }
            foundOptimalSolution = !stop;
        }
        long sec = (System.currentTimeMillis() - start) / 1000;
        logger.info("\nFound optimal solution " + main.get(GRB.DoubleAttr.ObjVal) +
                " in " + sec + " seconds and " + cutCounter + " cover cuts.\nTime in main "
                + (int) runtimeInMain + " seconds.\nCut length: " + Arrays.toString(cutLengthCounter));
        handler.close();
        return solution;
    }
}
