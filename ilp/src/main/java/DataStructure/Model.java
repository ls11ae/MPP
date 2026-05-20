package DataStructure;


import com.gurobi.gurobi.GRBEnv;
import com.gurobi.gurobi.GRBException;
import com.gurobi.gurobi.GRBModel;

public abstract class Model {
    public GRBEnv env;
    public GRBModel model;
    public ModelType type;

    public Model(ModelType type) throws GRBException {
        env = new GRBEnv(true);
        env.start();
        model = new GRBModel(env);
        this.type = type;
    }

    public abstract void createModel(Instance instance, boolean writeModelToFile) throws GRBException;

    public abstract Solution solveModel(String[] parameters, boolean writeModelSolutionToFile) throws GRBException;

    public void clearModel() throws GRBException {
        model.dispose();
        env.dispose();
    }
}
