opendeco: opendeco.c deco.c deco.h schedule.c schedule.h output.c output.h
	gcc -O3 -lm -Wall -Werror opendeco.c deco.c schedule.c output.c -o opendeco

run: opendeco
	./opendeco

clean:
	rm opendeco
