VERSION=\"$(shell git describe --tags --dirty)\"

opendeco: opendeco.c deco.c deco.h schedule.c schedule.h output.c output.h
	gcc --std=c99 -pedantic -Wall -Werror -O3 -DVERSION=${VERSION} opendeco.c deco.c schedule.c output.c -lm -o opendeco

run: opendeco
	./opendeco -d 30 -t 120 -g EAN32 --decogasses EAN50

clean:
	rm opendeco
