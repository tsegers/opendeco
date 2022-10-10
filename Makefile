VERSION=\"$(shell git describe --tags --dirty)\"

PREFIX = /usr/local

all: opendeco opendeco_test

opendeco: src/opendeco.c src/deco.c src/deco.h src/schedule.c src/schedule.h src/output.c src/output.h
	gcc --std=c99 -pedantic -Wall -Werror -O3 -DVERSION=${VERSION} src/opendeco.c src/deco.c src/schedule.c src/output.c -lm -o opendeco

opendeco_test: test/opendeco_test.c test/deco_test.c src/deco.c 
	gcc --std=c99 -pedantic -Wall -Werror -O3 test/minunit.c test/opendeco_test.c test/deco_test.c src/deco.c -lm -o opendeco_test

run: opendeco
	./opendeco -d 30 -t 120 -g EAN32 --decogasses EAN50

test: opendeco_test
	./opendeco_test

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f opendeco ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/opendeco

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/opendeco

clean:
	rm opendeco
	rm opendeco_test
