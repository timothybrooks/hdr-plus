from subprocess import call

for burst in xrange(38):

	command = "../tools/dcraw -w -k 2050 -g 2.4 12.92 -S 15464 -W /afs/cs/academic/class/15769-f16/project/tebrooks/raws/burst" + str(burst) + "_0.CR2"

	call(command, shell=True)

