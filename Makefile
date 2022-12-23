PREFIX = /usr/local

VERSION = \"$(shell git describe --tags --dirty)\"
CFLAGS = --std=c99 -pedantic -Wall -Werror -Os -DVERSION=${VERSION}

all: opendeco opendeco_test libopendeco.a

opendeco: src/opendeco.c src/deco.c src/deco.h src/schedule.c src/schedule.h src/output.c src/output.h src/opendeco-cli.h src/opendeco-cli.c src/opendeco-conf.h
	$(CC) $(CFLAGS) src/opendeco.c src/deco.c src/schedule.c src/output.c src/opendeco-cli.c -lm -o opendeco

opendeco_test: test/opendeco_test.c test/deco_test.c src/deco.c 
	$(CC) $(CFLAGS) test/minunit.c test/opendeco_test.c test/deco_test.c src/deco.c -lm -o opendeco_test

libopendeco.a:
	$(CC) -c src/deco.c -o src/deco.o
	$(CC) -c src/schedule.c -o src/schedule.o
	$(CC) -c src/output.c -o src/output.o
	ar rs libopendeco.a src/deco.o src/schedule.o src/output.o

lib: libopendeco.a

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
	rm -f opendeco
	rm -f opendeco_test
	rm -f src/*.o
	rm -f libopendeco.a
