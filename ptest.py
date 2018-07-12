
from pulp import *
import random

WEIGHT = "weight"
PRICE = "price"

players = \
	[
		"0",
		"1",
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9",
	]
	
weights = \
	{
		"0":1.0,
		"1":1.0,
		"2":2.0,
		"3":2.0,
		"4":3.0,
		"5":3.0,
		"6":4.0,
		"7":4.0,
		"8":5.0,
		"9":5.0,
	}
costs =   \
	{
		"0":1,
		"1":1,
		"2":2,
		"3":2,
		"4":3,
		"5":3,
		"6":4,
		"7":4,
		"8":5,
		"9":5,
	}
	
	
#for i in range(0,500):
#	players[str(i)] = {WEIGHT: random.randint(0,10), PRICE: random.randint(0,10)}

prob = LpProblem("s14", LpMaximize)	

vars = LpVariable.dicts("players",players,0,1,LpInteger)

# objective
prob += lpSum([vars[i] * weights[i] for i in players]), "weight of team"

# constraints
prob += lpSum(vars[i] for i in players) == 3, "team size"
prob += lpSum(vars[i] * costs[i] for i in players) <= 10, "budget"

print prob
prob.solve(COINMP_DLL())

for v in prob.variables():
	print v.name, "=", v.varValue


