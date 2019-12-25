Csc 360 
Assignment: 1
Dallas Brooks
V00868024

To compile program:
	make
To run program:
	./Pman

6 commands can be given to PMan:
	1. bg <cmd>: starts <cmd> in background
	2. bglist: lists all <process id>
	3. bgkill <process id>:	terminates <process id>
	4. bgstop <process id>: stops <process id>
	5. bgstart <process id>: starts <process id>
	6. pstat <process id>: lists comm, state, utime, stime, rss, voluntary_ctxt_switches, and nonvoluntary_ctxt_switches for <process id>