from subprocess import call

# 10 elems per line for easy counting

#          0            1            2            3            4            5            6            7            8            9 
params = [(4.2, 1.05), (3.8, 1.30), (3.9, 1.10), (3.8, 1.10), (5.0, 1.00), (3.8, 1.50), (3.6, 1.10), (4.0, 1.15), (4.0, 1.00), (3.4, 1.35),
#          10           11           12           13           14           15           16           17           18           19 
		  (3.8, 1.45), (4.8, 1.20), (3.6, 1.25), (3.6, 1.25), (4.5, 1.40), (2.0, 1.75), (3.8, 1.00), (3.8, 1.20), (3.6, 1.40), (3.8, 1.42),
#          20           21           22           23           24           25           26           27           28           29 
		  (2.0, 1.45), (3.8, 1.45), (3.5, 1.35), (3.2, 1.25), (3.8, 1.20), (3.5, 1.20), (3.8, 1.35), (4.5, 1.45), (3.8, 1.35), (4.0, 1.30), 
#          30           31           32           33           34           35           36           37           38           
		  (4.0, 1.45), (3.0, 1.35), (3.6, 1.30), (3.8, 1.30), (3.2, 1.30), (3.5, 1.40), (3.8, 1.40), (3.8, 1.55), (3.8, 1.55)]

# indices of bursts to skip (useful while tuning params)

skip = []

commands = []

for burst in xrange(38):

	if burst in skip: continue
	
	command = "./hdrplus"

	(comp, gain) = params[burst]
	
	command += (" -c " + str(comp) + " -g " + str(gain))

	command += " /afs/cs/academic/class/15769-f16/project/tebrooks/raws/"

	command += (" /outputs/output" + str(burst) + ".png")

	for img in xrange(8): 

		command += (" burst" + str(burst) + "_" + str(img) + ".CR2")

	commands += [command]

for command in commands:

	call(command, shell=True)

