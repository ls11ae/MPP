package Main;

import ContinuousModels.*;
import DataStructure.*;
import DiscreteModels.CC;
import Other.Panel;
import Other.Parser;
import com.gurobi.gurobi.GRBException;

import java.util.Arrays;
import java.util.Comparator;

/**
 * No executable main methode was created, as the main application was to create the models from the instances
 * and rum them via Gurobi command line tool.
 */
public class Main {
    public static final double EPSILON = 0.00000001;


    public static void main(String[] args) throws GRBException {
        double scale = 1.0;
        ModelType type = ModelType.HS2;
        String inputFile = "./Instances/Terashima/TS002C5.json";
        String[] parameters = {"TimeLimit=3600"};
        boolean writeModelToFile = false;
        boolean writeModelSolutionToFile = false;

        Instance instance = Parser.readInstanceFromJsonFile(inputFile, scale);
        Arrays.sort(instance.items, Comparator.comparingDouble(item -> -item.area));

//        for (ModelType t : new ModelType[]{ModelType.NFP_CM, ModelType.I_NFP_CM, ModelType.HS2, ModelType.NFP_CM_VS}) {
//            Model model = selectModel(t);
//            model.createModel(instance, true);
//            model.clearModel();
//        }

        Model model = selectModel(type);
        model.createModel(instance, writeModelToFile);
        Solution solution = model.solveModel(parameters, writeModelSolutionToFile);
//
//        SplitModel2 model = new SplitModel2(type);
//        model.createModel(instance);
//        Solution solution = model.solveModel(3600);

        Arrays.sort(instance.items, Comparator.comparingDouble(item -> item.index));
        if (solution != null) {
            Panel panel = new Panel(instance, solution);
            panel.drawSolution();
            Parser.writeSolutionToFile(solution);
        }

    }

    public static Model selectModel(ModelType type) throws GRBException {
        return switch (type) {
            case NFP_CM -> new NFP_CM();
            case I_NFP_CM -> new I_NFP_CM();
            case NFP_CM_VS -> new NFP_CM_VS();
            case HS2 -> new HS2();
            case CC -> new CC();
        };
    }
}
