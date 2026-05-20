package ContinuousModels;

import DataStructure.*;
import Main.Main;
import com.gurobi.gurobi.*;

import java.io.File;
import java.io.IOException;
import java.util.logging.FileHandler;
import java.util.logging.Logger;
import java.util.logging.SimpleFormatter;


/**
 * This version of the split model uses callbacks to check if a subset is a cover. This means
 * the subset is just a possible solution that fulfills the previous knapsack-inequalities, but
 * the value could be lower than the optimal solution (more subsets are checked than necessary).
 * And each cover is reduced to a minimum cover.
 * => for the small instances this version is slower
 */
public class SplitModel extends GRBCallback {
    ContinuousModel support;

    GRBModel main;
    GRBVar[][] packed;
    double bestSolution = 0.0;
    GRBLinExpr greater;
    Logger logger;
    long start;
    int cutCounter = 0;
    int removedVarsFromCoverCut = 0;
    Solution solution = null;

    public SplitModel(ModelType supportModel) throws GRBException {
        support = (ContinuousModel) Main.selectModel(supportModel);
        main = new GRBModel(support.env);
        logger = Logger.getAnonymousLogger();
//        logger.setUseParentHandlers(false);
    }

    @Override
    protected void callback() {
        try {
            if (where == GRB.CB_MIPSOL) {
                // get the subset from the main model
                Polygon[] items = support.instance.items;
                StringBuilder mainItemsAsString = new StringBuilder();
                int numItems = 0;
                double mainValue = 0.0;
                int[] indices = new int[items.length];
                boolean[][] in = new boolean[packed.length][];
                for (int i = 0; i < packed.length; i++) {
                    in[i] = new boolean[packed[i].length];
                    for (int j = 0; j < in[i].length; j++) {
                        in[i][j] = getSolution(packed[i][j]) > 0.5;
                        support.packed[i][j].set(GRB.DoubleAttr.UB, 0.0);
                        if (in[i][j]) {
                            mainItemsAsString.append("p_").append(items[i].index).append('_').append(j).append(" ");
                            numItems++;
                            mainValue += items[i].value;
                            indices[i]++;
                        }
                    }
                }

                logger.info("\n[" + ((System.currentTimeMillis() - start) / 1000) +
                        "]\tMain solution with value " + mainValue + "\nTesting "
                        + numItems + " items: \t\t" + mainItemsAsString);
                GRBLinExpr objective = new GRBLinExpr();
                GRBLinExpr coverCut = new GRBLinExpr();

                int subSetSize = 0;
                int value = 0;
                StringBuilder supportItemsAsString = new StringBuilder();
                // add the item one after the other until they no longer fit => knapsack cover
                for (int i = 0; i < indices.length; i++) {
                    for (int j = 0; j < indices[i]; j++) {
                        subSetSize++;
                        value += support.instance.items[i].value;
                        supportItemsAsString.append("p_").append(items[i].index).append('_').append(j).append(" ");
                        objective.addTerm(1.0, support.packed[i][j]);
                        coverCut.addTerm(1.0, packed[i][j]);
                        support.packed[i][j].set(GRB.DoubleAttr.UB, 1.0);
                        support.model.setObjective(objective, GRB.MAXIMIZE);
                        support.model.optimize();
                        if (support.model.get(GRB.DoubleAttr.ObjVal) - subSetSize < -0.1) {
                            // subset is a knapsack cover
                            // test for each item: if the cover is still infeasible without the item
                            // then the item doesn't belong to a minimum cover and is removed
                            subSetSize--;
                            while (i > 0) {
                                if (j < 0) {
                                    j = indices[--i] - 1;
                                }
                                while (j >= 0) {
                                    support.packed[i][j].set(GRB.DoubleAttr.UB, 0.0);
                                    support.model.optimize();
                                    if (Math.abs(support.model.get(GRB.DoubleAttr.ObjVal) - subSetSize) < 0.1) {
                                        support.packed[i][j].set(GRB.DoubleAttr.UB, 1.0);
                                        j = -1;
                                    } else {
                                        subSetSize--;
                                        coverCut.remove(packed[i][j]);
                                        removedVarsFromCoverCut++;
                                        indices[i]--;
                                        j--;
                                    }
                                }
                            }
                            supportItemsAsString = new StringBuilder();
                            int num = 0;
                            subSetSize++;
                            for (int k = 0; k < indices.length; k++) {
                                for (int l = 0; l < indices[k]; l++) {
                                    supportItemsAsString.append("p_").append(items[k].index).
                                            append("_").append(l).append(" ");
                                    if (num == subSetSize) {
                                        break;
                                    }
                                    num++;
                                }
                                if (num == subSetSize) {
                                    break;
                                }
                            }
                            addLazy(coverCut, GRB.LESS_EQUAL, coverCut.size() - 1);
                            logger.info("Added cover cut (" + (++cutCounter) + "):\t" + supportItemsAsString);
                            main.update();
                            return;
                        } else {
                            if (value > bestSolution) {
                                addLazy(greater, GRB.GREATER_EQUAL, value);
                                main.update();
                                bestSolution = value;
                                logger.info("Better feasible solution with value "
                                        + value + "\nItems:\t\t\t\t\t" + supportItemsAsString);
                                solution = support.getSolution();
                            }
                        }
                    }
                }
            }
        } catch (GRBException exception) {
            exception.printStackTrace();
        }
    }

    public void createModel(Instance instance) throws GRBException {

        packed = new GRBVar[instance.num_items][];
        GRBLinExpr objective = new GRBLinExpr();
        GRBLinExpr areaCut = new GRBLinExpr();
        greater = new GRBLinExpr();
        for (int i = 0; i < instance.num_items; i++) {
            Polygon item = instance.items[i];
            packed[i] = new GRBVar[item.quantity];
            for (int j = 0; j < item.quantity; j++) {
                GRBVar v = main.addVar(0.0, 1.0, 0.0, GRB.BINARY, "p_" + i + "_" + j);
                packed[i][j] = v;
                objective.addTerm(item.value, v);
                areaCut.addTerm(item.area, v);
                greater.addTerm(item.value, v);
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
        main.set(GRB.IntParam.LazyConstraints, 1);
        main.setCallback(this);
        main.set(GRB.IntParam.Threads, 1);
        main.set(GRB.DoubleParam.MIPGap, 0);
        main.set(GRB.IntParam.MIPFocus, 2);
        main.set(GRB.DoubleParam.TimeLimit, timeLimitInSeconds);
        support.model.set(GRB.IntParam.LogToConsole, 0);

        logger.info("Instance type: " + support.instance.type +
                "\nInstance name: " + support.instance.instance_name +
                "\nModel type: " + support.type +
                "\nTime limit (secs): " + timeLimitInSeconds + "\n\n");

        start = System.currentTimeMillis();
        main.optimize();
        long sec = (System.currentTimeMillis() - start) / 1000;
        logger.info("\nFound optimal solution " + main.get(GRB.DoubleAttr.ObjVal) + " in " + sec +
                " seconds and " + cutCounter + " cover cuts (removed variables: " + removedVarsFromCoverCut + ").\n");
        handler.close();
        return solution;
    }
}
