VERSION=\"$(shell git describe --tags --dirty)\"

PREFIX = /usr/local

all: opendeco test/opendeco_test

opendeco: opendeco.c deco.c deco.h schedule.c schedule.h output.c output.h
	gcc --std=c99 -pedantic -Wall -Werror -O3 -DVERSION=${VERSION} opendeco.c deco.c schedule.c output.c -lm -o opendeco

test/opendeco_test: deco.c test/opendeco_test.c
	gcc --std=c99 -pedantic -Wall -Werror -O3 test/minunit.c test/opendeco_test.c test/deco_test.c deco.c -lm -o test/opendeco_test

run: opendeco
	./opendeco -d 30 -t 120 -g EAN32 --decogasses EAN50

test: test/opendeco_test
	./test/opendeco_test

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f opendeco ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/opendeco

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/opendeco

clean:
	rm opendeco
	rm test/opendeco_test
