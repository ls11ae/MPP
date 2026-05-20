package DataStructure;

import com.gurobi.gurobi.GRBVar;

import java.io.*;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.TimeUnit;

/**
 * Data structure for calculation the edge clique cover with the ECC8 algorithm.
 */
public class ConflictGraph {
    ArrayList<Edges> edges = new ArrayList<>();
    int nodeCounter = 0;
    HashMap<Integer, Integer> nodeAndDegree = new HashMap<>();
    HashMap<Integer, GRBVar> idToVar = new HashMap<>();

    public void addEdge(int u, int v, GRBVar uVar, GRBVar vVar) {
        edges.add(new Edges(u, v));
        addNode(u, uVar);
        addNode(v, vVar);
    }

    private void addNode(int id, GRBVar var) {
        if (nodeAndDegree.containsKey(id)) {
            nodeAndDegree.put(id, nodeAndDegree.get(id) + 1);
        } else {
            idToVar.put(id, var);
            nodeAndDegree.put(id, 1);
            nodeCounter++;
        }
    }
    public int getSize() {
        return edges.size();
    }

    /**
     * Runs the ECC8 algorithm on the conflict graph.
     * @return array with all clique inequalities
     */
    public GRBVar[][] getCliqueCover() {
        if (nodeCounter == 0) {
            return new GRBVar[0][];
        }
        try {
            // creates a .nde file from the graph as input for the ECC8
            createCliqueCoverFiles();
            edges = null;
            System.gc();
            runECC8();

        } catch (IOException | InterruptedException e) {
            throw new RuntimeException(e);
        }


        // ECC8 writes the output in 4 files
        File file = new File(".\\ECC8\\graph.nde-rand.EPSc-stats.txt");
        int n = 0;
        try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (line.startsWith("Cliques")) {
                    String number = line.split(" ")[1];
                    n = Integer.parseInt(number);
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
            System.exit(-1);
        }
        file.delete();
        GRBVar[][] cliques = new GRBVar[n][];
        file = new File(".\\ECC8\\graph.nde-rand.EPSc.cover");
        try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
            for (int i = 0; i < n; i++) {
                String line = reader.readLine();
                int[] ids = Arrays.stream(line.split(" ")).mapToInt(Integer::parseInt).toArray();
                cliques[i] = new GRBVar[ids.length];
                for (int j = 0; j < ids.length; j++) {
                    cliques[i][j] = idToVar.get(ids[j]);
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
            System.exit(-1);
        }
        file.delete();
        new File(".\\ECC8\\graph.nde-rand.EPSc-nci.csv").delete();
        new File(".\\ECC8\\graph.nde-rand.EPSc-eci.csv").delete();
        new File(".\\ECC8\\graph.nde-rand.EPSc-clq.csv").delete();
        new File(".\\ECC8\\graph.nde").delete();
        return cliques;
    }

    private void createCliqueCoverFiles() throws IOException {
        File file = new File("./ECC8/graph.nde");
        file.createNewFile();
        BufferedWriter writer = new BufferedWriter(new FileWriter(file));
        writer.write(nodeCounter + "\n");

        for (Map.Entry<Integer, Integer> map : nodeAndDegree.entrySet()) {
            writer.write(map.getKey() + " " + map.getValue() + "\n");
        }
        for (Edges edge : edges) {
            writer.write(edge.toString() + "\n");
        }
        writer.close();
    }

    private void runECC8() throws InterruptedException, IOException {

        Process jar = new ProcessBuilder("java", "-jar", "./ECC8/ECC8.jar",
                "-g", "./ECC8/graph.nde", "-o", "./ECC8", "-f", "nde").start();

        // if the graph has more than 2 million edges ECC8 doesn't terminate, even if he found an edge clique cover
        // => use interrupt
//        jar.waitFor(60L, TimeUnit.SECONDS);
        jar.waitFor();
        jar.destroy();
    }



    private static class Edges {
        int u, v;

        public Edges(int u, int v) {
            this.u = u;
            this.v = v;
        }

        @Override
        public String toString() {
            return u + " " + v;
        }
    }
}


