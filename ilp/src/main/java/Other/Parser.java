package Other;

import DataStructure.Instance;
import DataStructure.Point;
import DataStructure.Polygon;
import DataStructure.Solution;
import com.google.gson.*;
import com.google.gson.stream.JsonReader;

import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.reflect.Type;

/**
 * Class for reading the instances and outputting the solutions as json files.
 * It is not check weather the input format is correct.
 */
public class Parser {
    private static final Gson gson = new GsonBuilder().setPrettyPrinting().
            registerTypeAdapter(Polygon.class, new Deserializer()).create();
    private static double scale = 1.0;
    private static int index = 0;

    public static Instance readInstanceFromJsonFile(String inputFile, double scale) {
        Parser.scale = scale;
        Parser.index = 0;
        Instance instance = null;
        try (FileReader fileReader = new FileReader(inputFile)) {
            JsonReader reader = new JsonReader(fileReader);
            instance = gson.fromJson(reader, Instance.class);
        } catch (IOException exception) {
            System.out.println("Couldn't read instance from input file.");
            System.exit(-1);
        }
        return instance;
    }

    public static void writeSolutionToFile(Solution solution) {
        for (int i = 0; i < solution.num_included_items; i++) {
            solution.x_translations[i] /= scale;
            solution.y_translations[i] /= scale;
        }
        String dirPath = "./Results/Solutions/" + solution.type;
        new File(dirPath).mkdirs();
        try (FileWriter writer = new FileWriter(dirPath + "/"+ solution.instance_name +
                "_sol_" + solution.total_value + ".json")) {
            gson.toJson(solution, writer);
        } catch (IOException exception) {
            System.out.println("Couldn't write solution in " + dirPath);
            System.exit(-1);
        }
    }

    static class Deserializer implements JsonDeserializer<Polygon> {

        @Override
        public Polygon deserialize(JsonElement json, Type type,
                                   JsonDeserializationContext jsonDeserializationContext) throws JsonParseException {
            JsonObject obj = json.getAsJsonObject();
            JsonArray jsonX = obj.get("x").getAsJsonArray();
            JsonArray jsonY = obj.get("y").getAsJsonArray();
            int n = jsonX.size();
            Point[] coordinates = new Point[n];
            for (int i = 0; i < n; i++) {
                coordinates[i] = new Point(jsonX.get(i).getAsDouble() * scale, jsonY.get(i).getAsDouble() * scale);
            }
            if (isContainer(obj)) {
                return new Polygon(coordinates, -1, -1, -1);
            } else {
                return new Polygon(coordinates, obj.get("quantity").getAsInt(), obj.get("value").getAsInt(), index++);
            }
        }

        private boolean isContainer(JsonObject obj) {
            return obj.get("quantity") == null;
        }
    }
}