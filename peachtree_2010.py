
import wwwCall
import re
import string
import pickle
from pulp import *

#
# Requires Python 2.6
#

MAX_PLAYERS = 500
DEBUG_WEIGHT_CALC = False

# should probably get rid of the globals

round_nr = 15
#
# The player data structure looks like this:
#
'''

everything is a string unless otherwise noted

PID :	PID
		NAME
		PRICE (int)
		TEAM
		POSITION (one of the recognised positions)
		TOTAL_POINTS (int)
		WEIGHT (float)
		DEBUG: map of string:float components of the weight
		PERFORMANCE :	array of...
							ROUND_TITLE
							VERSUS_TEAM
							SCORE_TRY
							SETUP_TRY
							LINE_BREAK
							TURN_OVER
							KICK_GOAL
							GO_FORWARD
							CATCH_KICK
							MAKE_TACKLE
							WIN_SCRUM
							WIN_LINEOUT
							MISS_GOAL
							LOSE_POSSESSION
							GIVE_PENALTY
							POINTS
		PRICE_HISTORY : list of ints

'''

# player data dict keys
PID = "pid"
NAME = "name"
POSITION = "position"
PRICE = "price"
TEAM = "team"
PERFORMANCE = "performance"
PRICE_HISTORY = "price-history" # todo
DEBUG = "debug"

# calculated keys
TOTAL_POINTS = "total-points"
WEIGHT = "weight"

# performance data dict keys
ROUND_TITLE = "round-title"
VERSUS_TEAM = "versus-team"
SCORE_TRY = "score-try"
SETUP_TRY = "setup-try"
LINE_BREAK = "line-break"
TURN_OVER = "turn-over"
KICK_GOAL = "kick-goal"
GO_FORWARD = "go-forward"
CATCH_KICK = "catch-kick"
MAKE_TACKLE = "make-tackle"
WIN_SCRUM = "win-scrum"
WIN_LINEOUT = "win-lineout"
MISS_GOAL = "miss-goal"
LOSE_POSSESSION = "lose-possession"
GIVE_PENALTY = "give-penalty"
POINTS = "points"

# team situation
TEAM_VALUE = "team-value"
CASH_IN_BANK = "cash-in-bank"
TOTAL_VALUE = "total-value"
PROFIT_LOSS = "profit-loss"
TRADES = "trades"

# recognised positions
FRONT_ROW = "Front Row"
LOCK = "Lock"
LOOSE_FORWARD = "Loose Forward"
HALF = "Half"
CENTRE = CENTER = "Centre"
OUTSIDE_BACK = "Outside Back"

# testrugby mappings of player names
# Map= team-sheet-player-name :: testrugby.com-player-name
player_name_map = \
	{
		"Brando Vaalu": "Brando Va'lu",
		"Tamaiti Horua": "Tamati Horua",
#		"Josh Valentine": "",
		"Duane Vermeulen": "Duane  Vermeulen",
		"Ricky Januarie": "Enrico Januarie",
		"Sireli Naqelevuki": "Sireli Naqele Vuki",
		"Conrad Jantjes": "Conrad Jantes",
		"Alando Soakai": "Alando Soaki",
		"Kendrick Lynn": "Kenny Lynn",
#		"Scott Cowan": "",
		"Michael Paterson": "Micheal Paterson",
		"Kahn Fotuali`i": " 	Khan Fotuali'i",
		"Hikawera Elliot": "Hika Elliott",
		"Kevin O'Neill": "Kevin O'Neil",
		"Dwayne Sweeney": "Dwayne Sweeny",
		"Gerhard Mostert": "Gerhard  Mostert",
		"Dan Coles": "Dane Coles",
		"Johann Muller": "Johan Muller",
		"Francois Steyn": "Franscois Steyn",
		"Bismarck du Plessis": "Bismarck  du Plessis",
		"Jacques Botes": "Jaques Botes",
		"Isaia Toeava": "Isaia Toe'ava",
		"Michael Hobbs": "Micheal Hobbs",
#		"Winston Stanley": "",
		"Guthro Steenkamp": "Gurthro Steenkamp",
		"Derick Kunn": "Derick Kuun",
		"Tatafu Polota-Nau": "Tatafu Polatu-Nau",
		"Rob Horne": "Robert Horne",
		"Alfie Mafi": "Alf Mafi",
		"Pek Cowan": "Pekahou Cowan",
		"Dane Heylett-Petty": "Dane Haylett-Petty",
		"Josh Tatupu": "Josh Tatapu",
		"Heinrich Brussow": "Heinrich Brussouw",
		"Ashley Johnson": "Ashley Johnson", #???
		"Ryno Barnes": "Rayno Barnes",
		"Walti Vermeulen": "Waltie Vermeulen",
		"Wicus Blaauw": "Wicus Blaauw", #???
		"Tiaan Liebenberg": "Tiaan Liebenburg",
		"Dewaldt Duvenhage": "Dewaldt Duvenage",
		"Deon Fourie": "Deon Fourie", #???
		"Pieter Louw": "Pieter Louw", #???
		"Matthew Berquist": "Mat Berquist",
		"Hika Elliot": "Hika Elliott",
		"Jarrad Hoeata": "Jarrad Hoeta",
		"Hannes Franklin": "Hannes Franklin", #???
		"Ross Geldenhuys": "Ross Geldenhuys",
		"Jacques Lombaard": "Jaques Lombaard",
		"Willem Stoltz": "Willem Stoltz", #???
		"Franco van der Merwe": "Franco van der Merwe", #???
		"Jonathan Mokuena": "Jonathan Mokuena", #???
		"Doppies La Grange": "Doppies la Grange",
		"Deon van Rensburg": "Deon van Rensburg",
		"Tonderai Chavhanga": "Tonderai Chavhanga", #???
		"Earl Rose": "Earl Rose", #???
		"Todd Clever": "Todd Clever",
		"Willie Ripia": "Willy Ripia",
		"Steven Sykes": "Steven Sykes", #???
		"Andries Strauss": "Andries Strauss", #???
		"Odwa Ndungane": "Odwa Ndungane", #???
		"Craig Burden": "Craig Burden", #???
		"Jannie Du Plessis": "Jannie Du Plessis", #???
		"Willem Alberts": "Willem Alberts", #???
		"Josevata Rokocoko": "Joe Rokocoko",
		"Filo Paulo": "Filo Paulo", #???
		"Tii Paulo": "Ti'i Paulo",
		"Sosene Anesi": "Soseni Anesi",
		"Kane Douglas": "Kane Douglas", #???
	}

#---------------------
def log(s):
	print s

#---------------------
def login(url, competition, teamname, password):
	log("Logging in...")

	log("")
	log("competition: %s" % competition)
	log("team name: %s" % teamname)
	log("")

	# create the object
	http = wwwCall.wwwCall(url)
	# add the features you want (cookies,auth)
	http.setCookieFile("./wwwCall.cookie")
	# reaching a logging URL and saving the cookie
	http.post(competition,{"teamName" : teamname, "userPw" : password})
	# register the username/password for the basic authentification
	http.setAuthBasic("peachtree","6640baycircle")
	return http

#---------------------
# returns the raw html lines of the given page
def getPage(http, url_prefix, page):
	str = http.get(url_prefix+page).read()
	lines = string.split(str, "\n")
	return lines

#---------------------
# returns a dictionary of PID to player name
def buildPlayerList(http, url_prefix):
	log("Building player list...")
	PLAYERS_PER_PAGE = 50
	nr_pages = MAX_PLAYERS/PLAYERS_PER_PAGE

	result = {}

	for i in range(0,nr_pages):
		log("Parsing page "+str(i)+"...")
		page = getPage(http, url_prefix, "playerfind.asp?firstItemPos="+str(i*PLAYERS_PER_PAGE)+"&act=1&pid=0&pv=0&sk=1&pn=&pos=0")
		temp = parsePlayerFindPage(page)
		for pid in temp.keys():
			result[pid] = temp[pid]

	print "Found "+str(len(result.keys()))+" players."

	return result;

#---------------------
# parses the stupid 1,123,456 format into an integer
# strips off any optional leading '$'
def parseShittyNumberFormat(nr_str):
	if string.find(nr_str, '$') == 0:
		nr_str = nr_str[1:]
	str = string.replace(nr_str, ',', '')
	return int(str)

#---------------------
# given the pid, returns a list of available price history observations for the player
# the website generally only provides the last 24 days worth, but there's nothing we
# can do about that
def getPlayerPriceHistory(http, url_prefix, pid):

	result = []

	page = getPage(http, url_prefix, "playerprice.asp?pid="+pid)

	price_re = re.compile('<img src="images/bar_history.gif" width="\d*" height="\d*" alt="([0-9,]*)">')

	for line in page:
		s = string.strip(line)

		if s.startswith('<td width="408" height="119" align="left" valign="bottom">'):
			# this is the price history table, all on one line
			iter = price_re.finditer(s)
			try:
				while True:
					match = iter.next()
					price_str = match.group(1)
					result.append(parseShittyNumberFormat(price_str))
			except StopIteration:
				pass

	return result

#---------------------
# given the pid, returns a player data structure
def getPlayerData(http, url_prefix, pid, name):
	log("Getting data for player "+pid+"-'"+name+"'...")

	result = {}
	result[PID] = pid
	result[NAME] = name

	page = getPage(http, url_prefix, "playerinfo.asp?pid="+pid)
	price_re = re.compile('<td class="TableCell3">&nbsp;<span class="headings">Buy</span> now for <span class="headings"><a href="javascript:onClick=buyPlayer\\(\d*,\'.*, .*\',\'\\$.*\'\\)">\\$(.*)</a></span></td>')
	position_re = re.compile('<td class="TableCell3">&nbsp;Plays <span class="headings">(.*)</span></td>')
	team_re = re.compile('<td class="TableCell3"> &nbsp;For <span class="headings">(.*)</span></td>')
	perf_header_re = re.compile('TD colspan=16 align="left" valign="top" class="TableHead2">Stats Performance by Game</TD>')
	perf_row_re = re.compile('<TD align="right" class="TableCell\d">(\d*)</TD>')

	parsing_performance = False
	perf_table = []

	for line in page:
		s = string.strip(line)
		if not parsing_performance:
			match = price_re.search(s)
			if match is not None:
				result[PRICE] = parseShittyNumberFormat(match.group(1))
				continue
			match = position_re.search(s)
			if match is not None:
				result[POSITION] = match.group(1)
				continue
			match = team_re.search(s)
			if match is not None:
				result[TEAM] = match.group(1)
				continue
			match = perf_header_re.search(s)
			if match is not None:
				parsing_performance = True
				continue
		else:
			if line == "</TABLE>":
				parsing_performance = False
				continue
			perf_table.append(s)

	result[PERFORMANCE] = parsePerformanceTable(perf_table)
	result[PRICE_HISTORY] = getPlayerPriceHistory(http, url_prefix, pid)

	# calculate some player data
	points = 0
	for perf_row in result[PERFORMANCE]:
		points += int(perf_row[POINTS])
	result[TOTAL_POINTS] = points

	# debugging...
#	print "team: %s" % result[TEAM]
#	print "position: %s" % result[POSITION]
#	print "performance rows: %i" % len(result[PERFORMANCE])

	return result

#---------------------
# returns a list of performance data dictionaries
def parsePerformanceTable(table):

	iter = table.__iter__()

	result = []

	row_header_re = re.compile('<TR valign="top" class="TableCell\d">')
	null_row_re = re.compile('<TD align="left" colspan="16">&nbsp;<br>')
	end_table_re = re.compile('<TR valign="top">')

	while True:

		row = {}

		next_line = iter.next()
		while not row_header_re.match(next_line) and not end_table_re.match(next_line):
			next_line = iter.next()
			pass

		# we have reached the end of the player performance table
		if end_table_re.match(next_line):
			return result

		if null_row_re.match(iter.next()):
			# the player has no performance data as yet
			return result

		round_title = next_line
		versus_team = iter.next()
		row[ROUND_TITLE] = round_title
		row[VERSUS_TEAM] = versus_team
		row[SCORE_TRY] = parsePerfRow(iter.next())
		row[SETUP_TRY] = parsePerfRow(iter.next())
		row[LINE_BREAK] = parsePerfRow(iter.next())
		row[TURN_OVER] = parsePerfRow(iter.next())
		row[KICK_GOAL] = parsePerfRow(iter.next())
		row[GO_FORWARD] = parsePerfRow(iter.next())
		row[CATCH_KICK] = parsePerfRow(iter.next())
		row[MAKE_TACKLE] = parsePerfRow(iter.next())
		row[WIN_SCRUM] = parsePerfRow(iter.next())
		row[WIN_LINEOUT] = parsePerfRow(iter.next())
		row[MISS_GOAL] = parsePerfRow(iter.next())
		row[LOSE_POSSESSION] = parsePerfRow(iter.next())
		row[GIVE_PENALTY] = parsePerfRow(iter.next())
		row[POINTS] = parsePerfRow(iter.next())

		result.append(row)

	return result

#---------------------
# returns a string performance value extracted from the given row
def parsePerfRow(row):
	row_re = re.compile('<TD align="right" class="TableCell\d">(<b>)?(-\d*|\d*)(</b>)?</TD>')
	match = row_re.match(row)
	return match.group(2)

#---------------------
# parses a single page of html player output and returns a dictionary of PID to player name
def parsePlayerFindPage(page):
	result = {}
	regex = re.compile('<td class="TableCell3"><a href="playerinfo.asp\\?pid=(\d*)">(.*)</a></td>')

	for line in page:
		match = regex.search(string.strip(line))
		if match is not None:
			pid = match.group(1)
			name = match.group(2)
			result[pid] = name
			#return result #debug to get just 1 player, for testing

	return result

#---------------------
# sums up a players performance stat
def sumPerfStat(player, stat):
	result = 0
	for row in player[PERFORMANCE]:
		result += int(row[stat])
	return result

#---------------------
# returns an int representing how many games in a row this players team has a game on
def calcAvailability(player, fixtures):
	fixture_list = fixtures[player[TEAM]]

	result = 0
	start_index = round_nr-1
	for fixture in fixture_list[start_index:]:
		if fixture[0] is not None:
			result += 1
		else:
			break

	return result

#---------------------
# returns the cost gradient of the player (a float)
def calcPriceGradient(player):
	price_history = player[PRICE_HISTORY]
	if len(price_history) >= 2:
		return float(price_history[-1]-price_history[0]) / len(price_history)
	else:
		return 0

#==============================================================================
#
# The all-important weighting function
# Assigns a value to a player
#
def calcPlayerWeight(player, player_history, fixtures_this_year, situation, squad, team_sheets):

	for entry in player_history:
		if entry is None:
			entry = \
				{
					PID: "0",
					NAME: None,
					PRICE: 0,
					TEAM: None,
					POSITION: None,
					TOTAL_POINTS: 0,
					WEIGHT: 0.0,
					DEBUG: {},
					PERFORMANCE : [],
					PRICE_HISTORY : [],
				}

	result = 0.0
	player[DEBUG] = {}

	# weight last years results exponentially less as the season goes on
	base_historical_weight = 1.0/((round_nr*round_nr)+1)
	historical_weight = [base_historical_weight, base_historical_weight/2]

	#
	# total points in this year and last
	#
	for x in range(0,len(player_history)):
		entry = player_history[x]
		if entry is None:
			historical_points = 0
		else:
			historical_points = float(entry[TOTAL_POINTS]) / 2000.0
		i = (historical_points * historical_weight[x])
		result += i
		player[DEBUG]["historical_points ["+str(x)+"]"] = i

	points_this_year = float(player[TOTAL_POINTS]) / 2000.0

	i = (points_this_year * (1.0-base_historical_weight))
	result +=  i
	player[DEBUG]["points_this_year"] = i

	#
	# tries this year and last (0.05 because the try bonuses work out to 100
	# points per try, and 100/2000.0 = 0.05)
	#
	base_historical_tries_weight = 0.05
	historical_tries_weight = [base_historical_tries_weight, base_historical_tries_weight/5]

	for x in range(0,len(player_history)):
		entry = player_history[x]
		if entry is None:
			historical_tries = 0
		else:
			historical_tries = float(sumPerfStat(entry, SCORE_TRY)) * historical_tries_weight[x]
		i = (historical_tries * historical_weight[x])
		result += i
		player[DEBUG]["historical_tries ["+str(x)+"]"] = i

	tries_this_year = float(sumPerfStat(player, SCORE_TRY)) * 0.05

	i = (tries_this_year * (1.0-base_historical_weight))
	result += i
	player[DEBUG]["tries_this_year"] = i

	#
	# availability
	#
	availability = calcAvailability(player, fixtures_this_year)

	i = 0.0
	if availability == 0:
		i = -10.0
	elif availability < 4:
		i = float(availability) / 500.0
	else:
		i = float(availability) / 100.0

	result += i
	player[DEBUG]["availability"] = i

	#
	# price history: check for a positive gradient
	#
	base_historical_gradient_weight = 50000.0
	historical_gradient_weight = [base_historical_gradient_weight, base_historical_gradient_weight*1.5]
	gradient_history = []
	for x in range(0,len(player_history)):
		entry = player_history[x]
	if entry is None:
		historical_gradient = 0
	else:
		historical_gradient = calcPriceGradient(entry) / historical_gradient_weight[x]
	i = (historical_gradient * historical_weight[x])
	result += i
	player[DEBUG]["historical_gradient ["+str(x)+"]"] = i

	gradient_this_year = calcPriceGradient(player)
	gradient_this_year /= 20000.0

	if gradient_this_year > 0:
		i = (gradient_this_year * (1.0-base_historical_weight))
		result += i
		player[DEBUG]["price_gradient_this_year"] = i
	else:
		player[DEBUG]["price_gradient_this_year"] = 0.0

	#
	# current squad: weight players in the current squad higher depending on
	# how many trades remain. contrast to the availability weight
	#
	if squad.has_key(player[PID]):
		i = 1.0 - situation[TRADES]/12.0
		if i<0.0:
			i = 0.0
		player[DEBUG]["current-squad+trades"] = i
		result += i

	#
	# Weekly team sheets
	#
	if team_sheets.has_key(player[TEAM]):
		team_sheet = team_sheets[player[TEAM]]
		starting = False
		bench = False
		count = 0
		for p in team_sheet:
			if string.lower(p) == string.lower(player[NAME]):
				if count < 15:
					starting = True
				else:
					bench = True
	else:
		starting = False
		bench = False

	i = 0.0
	if not starting and not bench:
		i = -10.0
	elif not starting and bench:
		i = -1.2
	player[DEBUG]["TEAM_SHEET"] = i
	result += i

	# todo: detect out of position

	return result

#==============================================================================

#---------------------
def printPositions(pos, list):
	print " --- "+pos+" --- "
	for player in list:
		s = "%s - %.2f, $%i " % (player[NAME], player[WEIGHT], player[PRICE])
		if DEBUG_WEIGHT_CALC:
			s += str(player[DEBUG])
		print s

#---------------------
# fills in the WEIGHT field for each player
def calcPlayerWeights(players):

	fixtures2010 = loadFixtures("2010")
	players2008 = loadData("players2008.pickle")
	players2009 = loadData("players2009.pickle")
	situation = loadData("situation.pickle")
	squad = loadData("squad.pickle")
	team_sheets = getTeamSheets()

	# validate team sheets
	for team in team_sheets.keys():
		for player in team_sheets[team]:
			found_player = False
			for pid in players.keys():
				p = players[pid]

				player_name = player
				if player_name_map.has_key(player_name):
					player_name = player_name_map[player_name]

				db_name = string.strip(string.lower(p[NAME]))
				ts_name = string.strip(string.lower(player_name))
				if ts_name == db_name:
					found_player = True
			if not found_player:
				print "TEAM SHEET PLAYER NOT FOUND: [%s]"%player_name


	for pid in players.keys():
		player2008 = None
		if players2008.has_key(pid):
			player2008 = players2008[pid]

		player2009 = None
		if players2009.has_key(pid):
			player2009 = players2009[pid]

		player = players[pid]
		player[WEIGHT] = calcPlayerWeight(player, [player2009, player2008], fixtures2010, situation, squad, team_sheets)

#---------------------
def sumPlayers(players, field):
	result = 0
	for player in players:
		result += player[field]
	return result

#---------------------
def makeTeam(players, budget):

	calcPlayerWeights(players)

	front_rows = []
	locks = []
	loose_forwards = []
	halves = []
	centers = []
	outside_backs = []

	result_map = \
		{
			FRONT_ROW: front_rows,
			LOCK: locks,
			LOOSE_FORWARD: loose_forwards,
			HALF: halves,
			CENTER: centers,
			OUTSIDE_BACK: outside_backs,
		}

	#
	# The LP problem is formulated as follows:
	#
	# Let xi be an integer variable representing the player in the i'th position
	# in the player_list. xi is 1 if the player is selected, 0 otherwise.
	#
	# Let wi be the weight of player i, calculated by the weighting function
	#
	# Let ci be the cost of player i, downloaded from the website
	#
	# Maximise:
	#  sum of wi*xi for all i
	#
	# Subject to:
	#  sum of ci*xi for all i <= budget
	#  sum of xi for all i in each position == 2
	#
	player_list = []
	weights = {}
	costs = {}

	for pid in players.keys():
		player_list.append(pid)

	for pid in player_list:
		weights[pid] = players[pid][WEIGHT]
		costs[pid] = players[pid][PRICE]

	# split the players into position groups
	fr_list = {}
	lk_list = {}
	lf_list = {}
	hv_list = {}
	ct_list = {}
	ob_list = {}

	split_map = \
		{
			FRONT_ROW: fr_list,
			LOCK: lk_list,
			LOOSE_FORWARD: lf_list,
			HALF: hv_list,
			CENTER: ct_list,
			OUTSIDE_BACK: ob_list,
		}

	for pid in players.keys():
		p = players[pid]
		split_map[p[POSITION]][pid] = p

	prob = LpProblem("PEACHTREE", LpMaximize)
	vars = LpVariable.dicts("player_list",player_list,0,1,LpInteger)

	# objective
	prob += lpSum([vars[i] * weights[i] for i in player_list]), "weight of team"

	# constraints
	#prob += lpSum(vars[i] for i in player_list) == 12, "team size"
	prob += lpSum(vars[i] for i in fr_list) == 2, "FR max"
	prob += lpSum(vars[i] for i in lk_list) == 2, "LK max"
	prob += lpSum(vars[i] for i in lf_list) == 2, "LF max"
	prob += lpSum(vars[i] for i in hv_list) == 2, "HV max"
	prob += lpSum(vars[i] for i in ct_list) == 2, "CT max"
	prob += lpSum(vars[i] for i in ob_list) == 2, "OB max"
	prob += lpSum(vars[i] * costs[i] for i in player_list) <= budget, "budget"

	print ""
	print "+------------------------------------------------------------------+"
	print "|Begin puLP debug crap                                             |"
	print "+------------------------------------------------------------------+"
	prob.solve()
	print "+------------------------------------------------------------------+"
	print ""

	# display solution
	tc = 0
	tw = 0.0
	for pid in player_list:
		xi = vars[pid].varValue
		if xi == 1:
			player = players[pid]
			result_map[player[POSITION]].append(player)
			tc += player[PRICE]
			tw += player[WEIGHT]

	for position in [FRONT_ROW, LOCK, LOOSE_FORWARD, HALF, CENTER, OUTSIDE_BACK]:
		printPositions(position, result_map[position])

	print ""
	print "total cost: %i" %(tc)
	print "total weight: %f" %(tw)

#---------------------
# stores the given data, overwrites whatever is in the file
def storeData(data, file_name):
	file = open(file_name, "wb")
	pickle.dump(data, file, pickle.HIGHEST_PROTOCOL)

#---------------------
# loads data from the given file
def loadData(file_name):
	file = open(file_name, "rb")
	return pickle.load(file)

#---------------------
# Gets and stores all data from the S14 2008 competition
# Note that this does lots of http GETs takes a long time
def save2008Data():

	print "Getting and storing S14 2008 data"

	url = "http://www.testrugby.com"
	competition = "http://www.testrugby.com/s1408/default.asp"
	teamname = "peachtree"
	password = "6640baycircle"
	url_prefix = "http://www.testrugby.com/s1408/"

	http = login(url, competition, teamname, password)
	players_list = buildPlayerList(http, url_prefix)

	players = {}
	count = 0
	print "getting player data..."
	for pid in players_list.keys():
		count += 1
		players[pid] = getPlayerData(http, url_prefix, pid, players_list[pid])
		if count%50 == 0:
			print "... got %i/%i player data ..." % (count, len(players_list))

	print "storing data..."
	storeData(players, "players2008.pickle")

#---------------------
# Gets and stores all data from the S14 2009 competition
# Note that this does lots of http GETs takes a long time
def save2009Data(get_player_data, get_team_data):

	print "Getting and storing S14 2009 data"

	url = "http://www.testrugby.com"
	competition = "http://www.testrugby.com/s1409/default.asp"
	teamname = "peachtree"
	password = "6640baycircle"
	url_prefix = "http://www.testrugby.com/s1409/"

	http = login(url, competition, teamname, password)

	#
	# Get player data
	#
	if get_player_data:
		players_list = buildPlayerList(http, url_prefix)

		players = {}
		count = 0
		print "getting player data..."
		for pid in players_list.keys():
			count += 1
			players[pid] = getPlayerData(http, url_prefix, pid, players_list[pid])
			if count%50 == 0:
				print "... got %i/%i player data ..." % (count, len(players_list))

		print "storing player data..."
		storeData(players, "players2009.pickle")

	#
	# Get team situation
	#
	if get_team_data:
		situation = getTeamSituation(http, url_prefix)
		storeData(situation, "situation.pickle")
		squad = getCurrentSquad(http, url_prefix)
		storeData(squad, "squad.pickle")

#---------------------
# Gets and stores all data from the S14 Season competition
# Note that this does lots of http GETs takes a long time
def saveSeasonData(year, url_dir, teamname, password, get_player_data, get_team_data, existing_player_data={}, players_list=None):

	print "Getting and storing S14 %s data" % (year)

	url = "http://www.testrugby.com"
	competition = "http://www.testrugby.com/%s/default.asp" % (url_dir)
	url_prefix = "http://www.testrugby.com/%s/" % (url_dir)

	http = login(url, competition, teamname, password)

	print "already have %i players data" % (len(existing_player_data))

	#
	# Get player data
	#
	if get_player_data or players_list is None:
		players_list = buildPlayerList(http, url_prefix)

		players = existing_player_data
		
		done = False
		while not done:
			try:
				count = 0
				nr_to_get = len(players_list)-len(existing_player_data)
				print "getting player data... (%i to get)" % (nr_to_get)

				for pid in players_list.keys():
					if not existing_player_data.has_key(pid):
						count += 1
						players[pid] = getPlayerData(http, url_prefix, pid, players_list[pid])
						print "storing..."
						storeData(players, "players"+year+".pickle")
						if count%50 == 0:
							print "... got %i/%i player data ..." % (count, nr_to_get)
			
				done = True
			except AttributeError:
				print "~~~~~~COMMS FAILURE~~~~~~~"
				pass

	#
	# Get team situation
	#
	if get_team_data:
		situation = getTeamSituation(http, url_prefix)
		storeData(situation, "situation.pickle")
		squad = getCurrentSquad(http, url_prefix)
		storeData(squad, "squad.pickle")
		
	return players_list

#---------------------
def addFixture(fixtures, home_team, away_team, venue):
	home_fixtures = []
	if fixtures.has_key(home_team):
		home_fixtures = fixtures[home_team]
	else:
		fixtures[home_team] = home_fixtures

	away_fixtures = []
	if fixtures.has_key(away_team):
		away_fixtures = fixtures[away_team]
	else:
		fixtures[away_team] = away_fixtures

	home_fixtures.append( (away_team, True, venue) )
	away_fixtures.append( (home_team, False, venue) )

#---------------------
# load the fixture list
def loadFixtures(year):
	file = open("fixtures"+year+".txt", "r")

	header_re = re.compile("^Week \d*$")
	fixture_re = re.compile("^\w* \d* - (\w*) v (\w*), (.*)$")
	bye_re = re.compile("^(\w*)/(\w*) Bye$")

	# Key: TEAM, Value: list of opposing (team, is_home, venue) tuples
	result = {}

	round_nr = 0
	for line in file.readlines():
		if header_re.match(line):
			round_nr += 1
			continue

		match = fixture_re.match(line)
		if match is not None:
			home_team = match.group(1)
			away_team = match.group(2)
			venue = match.group(3)

			addFixture(result, home_team, away_team, venue)
			continue

		match = bye_re.match(line)
		if match is not None:
			team1 = match.group(1)
			team2 = match.group(2)

			result[team1].append( (None, None, None) )
			result[team2].append( (None, None, None) )

		if match is None:
			print "NO MATCH ["+line+"]"

	return result

#---------------------
# Parses the My Situation page for the current team situation
def getTeamSituation(http, url_prefix):
	log("Getting team situation...")

	result = {}

	team_value_re = re.compile('<td width="40%" class="TableCell3">Team Value</td>')
	cash_in_bank_re = re.compile('<td class="TableCell3">Cash in Bank</td>')
	total_value_re = re.compile('<td class="TableCell3">Total Value</td>')
	profit_loss_re = re.compile('<td class="TableCell3">Profit/Loss</td>')
	trades_re = re.compile('<td class="TableCell3">Trades Left</td>')

	page = getPage(http, url_prefix, "situation.asp")
	iter = page.__iter__()

	while True:
		s = string.strip(iter.next())

		if team_value_re.match(s) is not None:
			iter.next()
			iter.next()
			result[TEAM_VALUE] = parseShittyNumberFormat(string.strip(iter.next()))
		elif cash_in_bank_re.match(s) is not None:
			iter.next()
			iter.next()
			result[CASH_IN_BANK] = parseShittyNumberFormat(string.strip(iter.next()))
		elif total_value_re.match(s) is not None:
			iter.next()
			iter.next()
			result[TOTAL_VALUE] = parseShittyNumberFormat(string.strip(iter.next()))
		elif profit_loss_re.match(s) is not None:
			iter.next()
			iter.next()
			try:
				result[PROFIT_LOSS] = parseShittyNumberFormat(string.strip(iter.next()))
			except ValueError:
				# means that the Profit/Loss probably says "even"
				result[PROFIT_LOSS] = 0
		elif trades_re.match(s) is not None:
			iter.next()
			iter.next()
			result[TRADES] = int(string.strip(iter.next()))
			# dodgy hack: we rely on this being the last found
			return result;

#---------------------
# Parses the Manage Squad page to get the current squad, as a map of PIDs
def getCurrentSquad(http, url_prefix):
	log("Getting current squad...")

	result = {}

	player_re = re.compile('<td align="right" class="TableCell\d*"><a href="javascript:onClick=sellPlayer\\((\d*),\'(.*, .*)\',\'\\$.*\'\\)">.*</a></td>')

	# for some reason this page is fill of \r's
	str = http.get(url_prefix+"squad.asp").read()
	str = string.replace(str, '\r', '\n') # dodgy html bullshit
	lines = string.split(str, '\n')
	page = lines

	for line in page:

		s = string.strip(line)

		match = player_re.match(s)
		if match is not None:
			pid = match.group(1)
			name = match.group(2)
			result[pid] = name

	return result

#---------------------
def getTeamSheets():
	log("Getting team sheets...")

	result = {}

	filename = "teamsheets%s.txt" % (round_nr)
	lines = open(filename,"r")

	iter = lines.__iter__()

	try:
		while True:
			team_name = string.strip(iter.next())
			team = []
			for i in range(0,22):
				player_name = string.split(iter.next(),":")[1]
				#player_name = iter.next()
				team.append(string.strip(player_name))
			result[team_name] = team
	except StopIteration:
		return result

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# MAIN
#save2008Data()
#save2009Data(True, True)


#------------------------------------------------------------------------------
'''
print "clearing 2010 player data..."
storeData({}, "players2010.pickle")
print "loading 2010 data..."
p2010 = loadData("players2010.pickle")
print "loaded %i players" % (len(p2010))
done = False
while not done:
	try:
		players_list = saveSeasonData("2010", "s1410", "peachtree", "6640baycircle", get_player_data=True, get_team_data=True, existing_player_data=p2010)
		done = True
	except AttributeError:
		print "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
		pass
'''
#------------------------------------------------------------------------------

#print "loading 2008 data..."
#p2008 = loadData("players2008.pickle")
#print "loaded %i players" % (len(p2008))

#print "loading 2009 data..."
#p2009 = loadData("players2009.pickle")
#print "loaded %i players" % (len(p2009))

#------------------------------------------------------------------------------
#'''
situation = loadData("situation.pickle")
print "loading 2010 data..."
p2010 = loadData("players2010.pickle")
print "loaded %i players" % (len(p2010))
print "total value: "+str(situation[TOTAL_VALUE])
makeTeam(p2010, situation[TOTAL_VALUE])
#'''
#------------------------------------------------------------------------------

#print getTeamSheets()

#print "loading fixtures..."
#f = loadFixtures("2010")
#print len(f.keys())
#print f

#getPlayerData(http, "http://www.testrugby.com/s1408/" "10514", 'Stefan Terblanche')

'''
print "loading 2008 and 2009 data..."
p2008 = loadData("players2008.pickle")
p2009 = loadData("players2009.pickle")

print p2009

print "comparing 08 and 09 data..."
count = 0
new_in_2009 = 0
for pid in p2009.keys():
	if p2008.has_key(pid):
		player2008 = p2008[pid]
		player2009 = p2009[pid]

		if player2008[NAME] == player2009[NAME]:
			count += 1
		else:
			print "mismatched pid %s: %s / %s" % (pid, player2008[NAME], player2009[NAME])
	else:
		new_in_2009 += 1

print "found %i matches" % (count)
print "new in 2009: %i" % (new_in_2009)
'''