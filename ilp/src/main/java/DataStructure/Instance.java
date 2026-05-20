package DataStructure;

/**
 * This class represents the test instance in the format of the 2024 CG:Challenge.
 */
public class Instance {
    public final String instance_name;
    public final Polygon container;
    public final Polygon[] items;
    public final int num_items;
    public final String type;


    public Instance(String instance_name, Polygon container, Polygon[] items, int num_items, String type) {
        this.instance_name = instance_name;
        this.container = container;
        this.items = items;
        this.num_items = num_items;
        this.type = type;
    }
}
