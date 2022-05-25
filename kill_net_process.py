from os import popen, kill
from sys import argv
from signal import SIGTERM
 

COMMAND = "netstat -lptn | grep "

WARNING = """Active Internet connections (only servers)"""

def close_all(filter_):
	process = popen(COMMAND + filter_)
	ret = process.read()
	if WARNING in ret:
		ret = ret.replace(WARNING, '')
	q = filter(lambda x: f in x, ret.split("\n"))
	p = next(q, None)
	count = 0
	while p:
		pid = list(filter(lambda x: x != '', p.split(" ")))
		if pid:
			pid = pid[-1]
			pid = pid.split("/")[0]
			print("Terminating ", pid)
			kill(int(pid), 9)
			count += 1
		else:
			print("Unable to parse -> ", pid)
		p = next(q, None)
	return count

if __name__ == "__main__":
	if len(argv) < 2:
		print("Use by <prog> <filter>\nWhere filter is grep arguments to filter the line uniquly")
		exit(0)
	f = " ".join(argv[1:])
	print("Executing: ", COMMAND, f)
	while close_all(f) > 0:
		continue
