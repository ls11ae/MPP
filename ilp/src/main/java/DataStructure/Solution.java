package DataStructure;

/**
 * This class represents the solution in the format of the 2024 CG:Challenge.
 * (except that the translations are doubles)
 */
public class Solution {
    public final String type, instance_name;
    public final int total_value, num_included_items;
    public final int[] item_indices;
    public final double[] x_translations, y_translations;
    public final ModelType model_type;

    public Solution(String type, String instance_name, ModelType model_type, int total_value, int num_included_items,
                    int[] item_indices, double[] x_translations, double[] y_translations) {
        this.type = type;
        this.instance_name = instance_name;
        this.model_type = model_type;
        this.total_value = total_value;
        this.num_included_items = num_included_items;
        this.item_indices = item_indices;
        this.x_translations = x_translations;
        this.y_translations = y_translations;
    }
}