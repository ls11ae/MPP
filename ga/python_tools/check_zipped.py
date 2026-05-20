from cgshop2024_pyutils import (
    verify,
    InstanceDatabase,
    InvalidSolution,
    BadSolutionFile,
    ZipSolutionIterator,
    ZipReaderError,
)

db = InstanceDatabase("cgshop2024_benchmark.zip")
sol_it = ZipSolutionIterator()
try:
    for solution in sol_it("../Solutions/leftlayer/leftlayer.zip"):
        print(solution["instance_name"])
        instance = db[solution["instance_name"]]
        try:
            value = verify(instance, solution)
        except InvalidSolution as inv_sol:
            print("Solution is invalid:", inv_sol)
            break
        except BadSolutionFile as bad_sol:
            print("Solution file is invalid:", bad_sol)
            break
        print("Solution is valid and has value", value)
except ZipReaderError as err:
    print("Error while reading zip file:", err)