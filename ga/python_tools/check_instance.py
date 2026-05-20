from cgshop2024_pyutils import (
    verify,
    InstanceDatabase,
    InvalidSolution,
    BadSolutionFile,
    parse_solution
)

import json

db = InstanceDatabase("cgshop2024_benchmark.zip")

# Opening JSON file
# f = open("../Solutions/jigsaw_cf3_xcd14250_28.json")
f = open("../Solutions/jigsaw_cf2_ebad6b02_335.json")
 
# returns JSON object as 
# a dictionary
data = json.load(f)
f.close()


solution = parse_solution(data)
instance = db[solution["instance_name"]]

try:
    value = verify(instance, solution)
except InvalidSolution as inv_sol:
    print("Solution is invalid:", inv_sol)
except BadSolutionFile as bad_sol:
    print("Solution file is invalid:", bad_sol)
else:
    print("Solution is valid and has value", value)