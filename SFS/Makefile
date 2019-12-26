.phony all:
all:
	gcc -Wall diskhelp.c diskinfo.c -o diskinfo
	gcc -Wall diskhelp.c disklist.c -o disklist
	gcc -Wall diskhelp.c diskget.c -o diskget
	gcc -Wall diskhelp.c diskput.c -o diskput
.phony clean:
clean:
	-rm diskinfo
	-rm disklist
	-rm diskget
	-rm diskput