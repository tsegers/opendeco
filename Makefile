opendeco: opendeco.c deco.c deco.h schedule.c schedule.h output.c output.h
	gcc --std=c99 -pedantic -Wall -Werror -O3 -lm opendeco.c deco.c schedule.c output.c -o opendeco

run: opendeco
	./opendeco

clean:
	rm opendeco
