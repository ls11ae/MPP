import clipper2.Clipper;
import clipper2.core.ClipType;
import clipper2.core.FillRule;
import clipper2.core.Paths64;
import clipper2.core.Point64;
import clipper2.core.Path64;
import clipper2.core.Rect64;

import java.awt.Color;
import java.awt.Graphics;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Random;
import java.util.Set;

import javax.swing.JFrame;
import javax.swing.JPanel;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.SerializationFeature;
import com.fasterxml.jackson.databind.node.ArrayNode;
import com.fasterxml.jackson.databind.node.ObjectNode;

import java.io.File;

/**
 * Algorithm for the atris-instances of the CG:SHOP 2024 Contest. (Maximum polygon packing with rectilinear blocks)
 * 
 *   
 * @author Alkan
 *
 */
class Main{
	// Parameters for initial items size and for rating system
	static double minPercentageToRemove = 0;
	static double maxPercentageToRemove = 0;
	static double toRemoveStep = 0.01;
	
	static boolean useRating = true;	// if false, set minRatingParameter = maxRatingParameter, else useless iterations will be made
	static double minRatingParameter = 1;
	static double maxRatingParameter = 1;
	static double ratingParameterStep = 0.01;
	
	// Decides in which corner to pack the blocks. (Only one should be true!)
	static boolean bottomLeft = true;
	static boolean topLeft = false;
	static boolean topRight = false;
	static boolean bottomRight = false;
	
	// width and height of the container
	static long containerWidth = 0;
	static long containerHeight = 0;
	
	// Strings needed for the JSON-File
	static String instance_name;
	static String type;
	
	public static void main(String[] args) throws FileNotFoundException, IOException, ParseException {
		
//		String jsonFilePath = "jsonFiles/instances/" + args[0] + ".cgshop2024_instance.json";
//		System.out.println("Gerade bei: " + args[0]);
		
		
		// Reading the input json-File
		//String jsonFilePath = "jsonFiles/instances/atris2812.cgshop2024_instance.json";
		String jsonFilePath = "jsonFiles/instances2/instance18_2.json";
		Object obj = new JSONParser().parse(new FileReader(jsonFilePath));
        JSONObject json = (JSONObject) obj; 
        
        instance_name = (String) json.get("instance_name");
        //type = "cgshop2024_solution";
        type = (String) json.get("type");
        
        JSONObject containerObject = (JSONObject) json.get("container");
        JSONArray containerX = (JSONArray) containerObject.get("x");
        JSONArray containerY = (JSONArray) containerObject.get("y");
        containerWidth =  (long) containerX.get(1);
        containerHeight = (long) containerY.get(2);
	
		long offset = 10;	// thickness and additional length for container walls
		
		// Bottom Container Rectangle
		Rectangle containerBottom = new Rectangle(-offset, -offset, containerWidth + (2 * offset), offset, -1);
		// Right Container Rectangle
		Rectangle containerRight = new Rectangle(containerWidth, -offset, offset, containerHeight + (2 * offset), -1);
		// Left Container Rectangle
		Rectangle containerLeft = new Rectangle(-offset, -offset, offset, containerHeight + (2 * offset), -1);
		// Top Container Rectangle
		Rectangle containerTop = new Rectangle(-offset, containerHeight, containerWidth + (2 * offset), offset, -1);
		
		List<Rectangle> placed = new ArrayList<>();		// list of placed rectangles 
		
		List<Item> originalItems = new ArrayList<>();	// list of all items
		
		JSONArray items = (JSONArray) json.get("items");
		
		int id = 0;
		
		// Creating Item-objects of each block and putting them in the originalItems-List 
        for(int i = 0; i < items.size(); i++) {
        	JSONObject blockObject = (JSONObject) items.get(i);
        	
        	long quantity = (long) blockObject.get("quantity");		
            long value = (long) blockObject.get("value");
            
            for(int q = 0; q < quantity; q++) {
            	JSONArray itemX = (JSONArray) blockObject.get("x");
                JSONArray itemY = (JSONArray) blockObject.get("y");
                
                List<Long> blockX = new ArrayList<>();
                List<Long> blockY = new ArrayList<>();
                int len = itemX.size();
                for(int j = 0; j < len; j++) {
                	blockX.add((long) itemX.get(j));
                	blockY.add((long) itemY.get(j));
                }
                
                List<Rectangle> rectangles = new ArrayList<>();
                rectangles = BlockDecomposer.getRects(blockX, blockY, id);	// Decompose blocks into rectangles
                
                if(rectangles != null && !rectangles.isEmpty()) {
                	Item block = new Item(rectangles, value, id); 
                	      	
                	// Checks if the block fits in the container
                	if(block.width <= containerWidth && block.height <= containerHeight) {
                		originalItems.add(block);
                	}
                	
                }
                
                id++;
            }
        }
        
        
        

        List<Item> usedItems = new ArrayList<>();		// list of all items in a solution
        
//     // for drawing the rectangles
//        JFrame frame = new JFrame("Rectangle Drawer");
//        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
//        //final double scaleFactor = 0.00005; // Skalierungsfaktor
//        final double scaleFactor = 4; // Skalierungsfaktor
//
//        JPanel panel = new JPanel() {
//            @Override
//            protected void paintComponent(Graphics g) {
//                super.paintComponent(g);
//
//                Random random = new Random();
//
//                for (Item item : usedItems) {
//                    //Color fillColor = getRandomColor();
//                	Color fillColor = new Color(135,206,255);
//                	
//                	if(item.id % 4 == 0) {
//                		fillColor = new Color(200,0,0);
//                	}else if(item.id % 4 == 1) {
//                		fillColor = new Color(0,200,0);
//                	}else if(item.id % 4 == 2) {
//                		fillColor = new Color(0,0,200);
//                	}else {
//                		fillColor = new Color(100,100,100);
//                	}
//
//                    for (Rectangle rect : item.rectangles) {
//                        int scaledWidth = (int) (rect.width * scaleFactor);
//                        int scaledHeight = (int) (rect.height * scaleFactor);
//                        int x = (int) (rect.x * scaleFactor);
//                        int y = (int) (getHeight() - rect.y * scaleFactor - scaledHeight);
//
//                        g.setColor(fillColor);
//                        g.fillRect(x, y, scaledWidth, scaledHeight);
//
//                        // Schwarze Border
//                        g.setColor(Color.BLACK);
//                        g.drawRect(x, y, scaledWidth, scaledHeight);
//                    }
//                }
//
//                int scaledWidth = (int) (containerWidth * scaleFactor);
//                int scaledHeight = (int) (containerHeight * scaleFactor);
//                int x = 0;
//                int y = getHeight() - scaledHeight;
//                g.setColor(Color.BLACK);
//                g.drawRect(x, y, scaledWidth, scaledHeight);
//            }
//        };
//
//        frame.add(panel);
//        frame.setSize(1600, 1000);
//        frame.setVisible(true);
        

        
        List<Item> bestSolution = new ArrayList<>();	
        long bestValue = 0;
        double bestPercentage = 0;
        double bestRatingParameter = 0;
        long bestTime = 0;
        
        
        for(double ratingParameter = minRatingParameter; ratingParameter <= maxRatingParameter; ratingParameter += ratingParameterStep) {
        	for(double removePercentage = minPercentageToRemove; removePercentage <= maxPercentageToRemove; removePercentage += toRemoveStep) {
        		usedItems.clear();
            	// DeepCopy of the list with all items because of multiple use (parameter changing)
            	List<Item> itemList = deepCopy(originalItems);
            	// Putting in all container walls first
            	placed.clear();
            	placed.add(containerBottom);
        		placed.add(containerTop);
        		placed.add(containerLeft);
        		placed.add(containerRight);
        		
        		// Round parameters to the second decimal place (because of inaccuracies)
        		ratingParameter = Math.round(ratingParameter * 100.0) / 100.0;
        		removePercentage = Math.round(removePercentage * 100.0) / 100.0;
            	
            	// For time measuring
        		long startTime = System.currentTimeMillis();
            	
            	// Sort in descending order by value/area ratio
                Collections.sort(itemList, Comparator.comparingDouble(block -> -(((double) block.value / block.area) * 100))); 
                // Size of the quantity to be removed
                int sizeToRemove = (int) (itemList.size() * removePercentage);
                // Remove the worse items
                itemList.subList(itemList.size() - sizeToRemove, itemList.size()).clear();
              
                
                // Best-Fit Algorithm
                // Calculate NFP for every item and the container walls
                for(Item block : itemList) {
                	Paths64 nfp = calculateNFP(placed, block);
                	block.nfp = nfp;
                }
                
                // Variables to norm distances and Value-Area-ratio (for rating system) 
                long maxDistance, minDistance;
                double maxRatio, minRatio;
                
                // Placing blocks one after the other until no block can be placed anymore
                while(true) {
                	List<CandidateDetails> candidates = new ArrayList<>();
                	maxDistance = 0; maxRatio = 0; minDistance = Long.MAX_VALUE; minRatio = Double.MAX_VALUE;
                	
                	for(Item block : itemList) {
                		Paths64 nfp = block.nfp;
                		// Calculate NFP with the latest placed item and union it with the saved NFP of the block
                		if(!usedItems.isEmpty()) {
                			nfp = calculateNFP(usedItems.get(usedItems.size() - 1).rectangles, block);		
                			nfp = Clipper.Union(nfp, block.nfp, FillRule.NonZero);
                			block.nfp = nfp;
                			
                			
                		}
                		
                		// Find best Position for the block (smallest distance to container corner)
                		Point64 bestPosition = getBestPosition(block);
                		
                		
            			
                		// Add block to candidate as a CandidateDetails-Object if block would fit
            			if(bestPosition != null) {
            				// Calculate distance to the corner
            				long distance = 0;
                        	if(bottomLeft) {
                        		distance = (long) Math.sqrt(Math.pow(bestPosition.x, 2) + Math.pow(bestPosition.y, 2));  //Bottom Left
                        	}else if(topLeft) {
                        		distance = (long) Math.sqrt(Math.pow(bestPosition.x, 2) + Math.pow(bestPosition.y + block.height - containerHeight, 2));	//Top Left
                        	}else if(topRight) {
                        		distance = (long) Math.sqrt(Math.pow(bestPosition.x + block.width - containerWidth, 2) + Math.pow(bestPosition.y + block.height - containerHeight, 2));	//Top Right
                        	}else if(bottomRight) {
                        		distance = (long) Math.sqrt(Math.pow(bestPosition.x + block.width - containerWidth, 2) + Math.pow(bestPosition.y, 2));		//Bottom Right
                        	}
                        	
                        	// Calculate value-area-ratio of the block
                        	double areaValue = (double) block.area / block.value * 100;
            				
                        	// Update min and max distances
            				if(distance < minDistance) {
            					minDistance = distance;
            				}
            				if(distance > maxDistance) {
            					maxDistance = distance;
            				}
            				
            				// Update min and max value-area-ratio
            				if(areaValue > maxRatio) {
            					maxRatio = areaValue;
            				}
            				if(areaValue < minRatio) {
            					minRatio = areaValue;
            				}
            				
            				// Add to candidate with the details as CandidateDetails
            				candidates.add(new CandidateDetails(bestPosition, block, distance, areaValue));
            			}
                	}

                	// If candidates is empty, no item fits anymore -> End Iteration
                	if(candidates.isEmpty()) {
                		break;
                	}
                	
                	// Create initial best candidate with inital rating
                	CandidateDetails bestCandidate = candidates.get(0);
                	double ratingBest = bestCandidate.ratio;
                	
                	// Iterating over every block which can be placed
                	for(CandidateDetails candidate : candidates) {
                		// Initial ratings (choose block with best value-area-ratio first)
                		double ratingCurrent = candidate.ratio;
                		// Calculate Rating
                		if((maxRatio - minRatio) == 0) {
                			ratingCurrent = candidate.dist;
                		}else if((maxDistance - minDistance) != 0 && (maxRatio - minRatio) != 0) {
                    		ratingCurrent = ratingParameter * (candidate.ratio - minRatio) * (1 /(maxRatio - minRatio)) +  (candidate.dist - minDistance) * ((double) 1 /(maxDistance - minDistance));
                		}

                		// if rating not used, choose block with shortest distance. Ratio as tiebreaker
                		if((useRating && ratingBest > ratingCurrent)) {			
                			bestCandidate = candidate;
                			ratingBest = ratingCurrent;
                		}else if(!useRating && (candidate.dist < bestCandidate.dist || (candidate.dist == bestCandidate.dist && candidate.ratio > bestCandidate.ratio))) {
                			bestCandidate = candidate;
                		}
                	}
                	

                	// Placing the best candidate (setting x and y for the block and it rectangles)
                	bestCandidate.block.x = (int) bestCandidate.point.x;
                	bestCandidate.block.y = (int) bestCandidate.point.y;
        			for(Rectangle rect : bestCandidate.block.rectangles) {
        				rect.x = (int) (bestCandidate.point.x + rect.relativeX);
        				rect.y = (int) (bestCandidate.point.y + rect.relativeY);
        				placed.add(rect);
        			}
        			
        			usedItems.add(bestCandidate.block);
        			itemList.remove(bestCandidate.block);
        			
        			//System.out.println("Amount placed blocks: " + usedItems.size());
        			
        			// Repaint after each step
        			//panel.repaint();
                }
                
                
                // Calculate value of the packing
        		long sumValue = 0;
        		for(Item a : usedItems) {
        			sumValue += a.value;
        		}
        		
        		// For time measuring
        		long endTime = System.currentTimeMillis();
        		long totalTime = endTime - startTime;
        		System.out.println("Laufzeit: " + totalTime + " Millisekunden");
        		
        		System.out.println("numItems: " + usedItems.size() + ", SumValue: " + sumValue + ", percentage: " + removePercentage + ", ratingParameter: " + ratingParameter + ", rating: " + useRating);  
        		
        		writeJSON(usedItems, sumValue, removePercentage, ratingParameter, totalTime);
        		
        		if(sumValue > bestValue) {
        			bestSolution = deepCopy(usedItems);
        			bestValue = sumValue;
        			bestPercentage = removePercentage;
        			bestRatingParameter = ratingParameter;
        			bestTime = totalTime;
        		}
        	
        	}
        	
        }
        

		System.out.println("Best Solution:");
		System.out.println("numItems: " + bestSolution.size() + ", SumValue: " + bestValue + ", percentage: " + bestPercentage + ", ratingParameter: " + bestRatingParameter + ", rating: " + useRating);  
		
		
        //panel.repaint();		

		
	}
	
	public static void writeJSON(List<Item> solution, long score, double removePercentage, double ratingParameter, long time) {
        // Creating JSON-file for the solution
        int num_included_items = solution.size();

        List<Integer> item_indices = new ArrayList<>();
        List<Integer> x_translations = new ArrayList<>();
        List<Integer> y_translations = new ArrayList<>();

        for (Item a : solution) {
            item_indices.add(a.id);
            x_translations.add((int) (a.rectangles.get(0).x - a.rectangles.get(0).relativeX));
            y_translations.add((int) (a.rectangles.get(0).y - a.rectangles.get(0).relativeY));
        }

        // Use Jackson ObjectMapper to create JSON structure
        ObjectMapper objectMapper = new ObjectMapper();
        objectMapper.enable(SerializationFeature.INDENT_OUTPUT);	// structure File to make it easier to read
        ObjectNode solutionObject = objectMapper.createObjectNode();
        
        
        solutionObject.put("algorithm-type", "Best-Fit for atris");
        solutionObject.put("Percentage removed", removePercentage);
        solutionObject.put("Rating Parameter", ratingParameter);
        solutionObject.put("running-time", time);
        solutionObject.put("Score", score);

        solutionObject.put("type", type);
        solutionObject.put("instance_name", instance_name);
        solutionObject.put("num_included_items", num_included_items);
        solutionObject.put("item_indices", objectMapper.valueToTree(item_indices));
        solutionObject.put("x_translations", objectMapper.valueToTree(x_translations));
        solutionObject.put("y_translations", objectMapper.valueToTree(y_translations));

        

        // Path for the folder in which the file should be saved in
        //String folderPath = "jsonFiles/solutions/" + instance_name + "/";
        String folderPath = "jsonFiles/solutions/";
        
        String removePercent = String.format("%.2f", removePercentage).replace('.', '-');
        String ratingPara = String.format("%.2f", ratingParameter).replace('.', '-');

        try {
            // Write JSON to file
            objectMapper.writeValue(new File(Paths.get(folderPath, instance_name + "_solution" + "_" + removePercent + "_" + ratingPara + ".json").toString()), solutionObject);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
	
	
	/**
	 * Method for calculating the NFP of a Block with a list of rectangles. For the calculation a simple method is used which only works for rectangles.
	 * The NFP of a rectangle with a rectangle is a rectangle. So the NFP of all rectangles from a block is calculated with every rectangle in placed, 
	 * moved to the correct position (relative positions of rectangles in blocks) and then the union is calculated.
	 * @param placed: list of Rectangles which are already placed. 
	 * @param block: The Item-block which NFP is calculated with the rectangles in placed.. 
	 * @return : Paths64 which describes the NFP of the block with all rectangles in placed.
	 */
	public static Paths64 calculateNFP(List<Rectangle> placed, Item block) {
		List<Rectangle> blockRects = block.rectangles;
		List<Paths64> paths = new ArrayList<>();
		
		// Calculate NFP of every block rectangle with every every rectangle in placed
		for(Rectangle blockRect : blockRects) {
			for(Rectangle placedRect : placed) {
				// Create NFP and move it to relative position
				Rectangle nfp = new Rectangle(-blockRect.width, -blockRect.height, blockRect.width + placedRect.width, blockRect.height + placedRect.height, -1);
				nfp.x += placedRect.x - blockRect.relativeX;
				nfp.y += placedRect.y - blockRect.relativeY;
				
				// Create Path of the NFP and save it in paths
				Paths64 p = new Paths64();
				p.add(Clipper.MakePath(new long[] { nfp.x, nfp.y, nfp.x + nfp.width, nfp.y, nfp.x + nfp.width, nfp.y + nfp.height, nfp.x, nfp.y + nfp.height}));
				paths.add(p);
				
			}
		}
		// Calculate union of all NFPs
		Paths64 result = new Paths64();
		for(Paths64 p : paths) {
			result = Clipper.Union(result, p, FillRule.NonZero);
		}

		return result;
	}
	
	/**
	 * Method to calculate the "best" placement for a block. Iterating over each corner of the NFPs of the given block and return the point, which distance to 
	 * the specific corner is the smallest. The result point is the point, where the bottom-left corner of the bounding-box of the block should be placed.
	 * @param block : The Item-block which best position is searched
	 * @return : the Point64, which describes the best position as a point with x- and y-coordinate. null if no point inside the container is found
	 */
	public static Point64 getBestPosition(Item block) {
		Paths64 paths = block.nfp;
		Point64 bestPoint = new Point64(Integer.MAX_VALUE, Integer.MAX_VALUE);
		long minDist = Long.MAX_VALUE;
		// Iterating over each corner of all NFPs
		for(Path64 path : paths) {
			for(Point64 point : path) {
				// Check if the block would lie inside the container
				if(point.y > -1 && point.y + block.height <= containerHeight && point.x > -1 && point.x + block.width <= containerWidth) {			
					long distance = 0;
					// Calculate shortest distance to one of the corners
					if(bottomLeft) {
						distance = (long) Math.sqrt(Math.pow(point.x, 2) + Math.pow(point.y, 2));		//Bottom Left
					}else if(topLeft) {
						distance = (long) Math.sqrt(Math.pow(point.x, 2) + Math.pow(point.y + block.height - containerHeight, 2));		//Top Left
					}else if(topRight) {
						distance = (long) Math.sqrt(Math.pow(point.x + block.width - containerWidth, 2) + Math.pow(point.y + block.height - containerHeight, 2));	//Top Right
					}else if(bottomRight) {
						distance = (long) Math.sqrt(Math.pow(point.x + block.width - containerWidth, 2) + Math.pow(point.y, 2));		//Bottom Right
					}
					// Saving new best, if found
					if(distance < minDist) {
						minDist = distance;
						bestPoint.x = point.x;
						bestPoint.y = point.y;
					}
				}
			}
		}
		// return null if no point inside container is found -> block does not fit
		if(bestPoint.x == Integer.MAX_VALUE) {
			//System.out.println("No valid placement for current block");
			return null;
		}
		return bestPoint;
	}
	
		
	/**
	 * Method for creating a DeepCopy of the original item list (for using it multiple times). A DeepCopy of all Items and their rectangles is created
	 * @param originalList : The list of items, which should be copied
	 * @return : A DeepCopy of the originalList
	 */
	private static List<Item> deepCopy(List<Item> originalList) {
		List<Item> copyList = new ArrayList<>();

        // Iterate over all items and create a DeepCopy
        for (Item originalItem : originalList) {
            Item copiedItem = new Item(originalItem);	// Created with the DeepCopy Constructor of Item
            copyList.add(copiedItem);
        }

        return copyList;      
	}	
		
}
