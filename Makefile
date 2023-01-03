VERSION = \"$(shell git describe --tags --dirty)\"

CC=gcc

IFLAGS = -I.
LFLAGS = -lm

CFLAGS = -Wall -Werror --std=c99 -pedantic $(IFLAGS) -D_DEFAULT_SOURCE -DVERSION=${VERSION}
LDFLAGS = $(LFLAGS)

PREFIX = /usr/local

OBJ_BIN = src/opendeco.o src/opendeco-cli.o src/opendeco-conf.o src/deco.o src/output.o src/schedule.o toml/toml.o
OBJ_LIB = src/deco.o src/output.o src/schedule.o
OBJ_TST = test/opendeco_test.o test/deco_test.o src/deco.o minunit/minunit.o

LICENSES = minunit/LICENSE.h toml/LICENSE.h

DEPS = $(shell find -type f -name "*.dep")

all: opendeco opendeco_test libopendeco.a

run: opendeco
	./opendeco -d 30 -t 120 -g EAN32 --decogasses EAN50

test: opendeco_test
	./opendeco_test

lib: libopendeco.a

install: opendeco
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f opendeco ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/opendeco

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/opendeco

opendeco: $(OBJ_BIN)
	@echo "  LD      $@"
	@$(CC) -o opendeco $(OBJ_BIN) $(LDFLAGS)

opendeco_test: $(OBJ_TST)
	@echo "  LD      $@"
	@$(CC) -o opendeco_test $(OBJ_TST) $(LDFLAGS)

libopendeco.a: $(OBJ_LIB)
	@ar rs libopendeco.a $(OBJ_LIB)

%.o: %.c # override the built-in default recipe
%.o: %.c $(LICENSES)
	@echo "  CC      $@"
	@mkdir -p .dep/$(dir $@)
	@$(CC) $(CFLAGS) -MD -MF .dep/$@.dep -o $@ -c $<

%LICENSE.h: %LICENSE
	@echo "  XXD     $@"
	@xxd -i $< > $@

clean:
	rm -f $(OBJ_BIN)
	rm -f $(OBJ_LIB)
	rm -f $(OBJ_TST)
	rm -f $(LICENSES)
	rm -f opendeco
	rm -f opendeco_test
	rm -f libopendeco.a
	rm -rf .dep

-include $(DEPS)

.PHONY: all run test lib install uninstall clean
