CC?=clang

FIND=$(shell which gfind find | head -1)
OBJCOPY=$(shell which objcopy)

SRC:=
INCLUDES:=
CFLAGS:=
LDFLAGS:=
DESTDIR?=/usr/local

SRC+=$(shell $(FIND) src/ -type f -name '*.c')
INCLUDES+=-Isrc
INCLUDES+=-Ilib/.dep/include
LIBS:=

LIBS+=lib/cofyc/argparse
SRC+=lib/cofyc/argparse/argparse.c

LIBS+=lib/emmanuel-marty/em_inflate
SRC+=lib/emmanuel-marty/em_inflate/lib/em_inflate.c

LIBS+=lib/erkkah/naett
SRC+=lib/erkkah/naett/naett.c
ifeq ($(OS),Windows_NT)
  LDFLAGS+=-lwinhttp
else
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Linux)
    LDFLAGS+=-lcurl -lpthread
  endif
  ifeq ($(UNAME_S),Darwin)
    LDFLAGS+=-framework Foundation
  endif
endif

LIBS+=lib/rxi/microtar
SRC+=lib/rxi/microtar/src/microtar.c

LIBS+=lib/tidwall/json.c
SRC+=lib/tidwall/json.c/json.c

OBJ:=$(SRC:.c=.o)
OBJ:=$(OBJ:.cc=.o)
OBJ+=license.o

CFLAGS+=${INCLUDES}

.PHONY: default
default: dep

license.o: LICENSE.md
	$(OBJCOPY) --input binary --output elf64-x86-64 --binary-architecture i386 $< $@

lib/cofyc/argparse:
	mkdir -p lib/cofyc/argparse
	curl -sL https://github.com/cofyc/argparse/archive/refs/heads/master.tar.gz | tar xzv --strip-components=1 -C lib/cofyc/argparse
	mkdir -p lib/.dep/include/cofyc
	ln -s ../../../cofyc/argparse/argparse.h lib/.dep/include/cofyc/argparse.h

lib/emmanuel-marty/em_inflate:
	mkdir -p lib/emmanuel-marty/em_inflate
	curl -sL https://github.com/emmanuel-marty/em_inflate/archive/refs/heads/master.tar.gz | tar xzv --strip-components=1 -C lib/emmanuel-marty/em_inflate
	mkdir -p lib/.dep/include/emmanuel-marty
	ln -s ../../../emmanuel-marty/em_inflate/lib/em_inflate.h lib/.dep/include/emmanuel-marty/em_inflate.h

lib/erkkah/naett:
	mkdir -p lib/erkkah/naett
	curl -sL https://github.com/erkkah/naett/archive/refs/heads/main.tar.gz | tar xzv --strip-components=1 -C lib/erkkah/naett
	mkdir -p lib/.dep/include/erkkah
	ln -s ../../../erkkah/naett/naett.h lib/.dep/include/erkkah/naett.h

lib/rxi/microtar:
	mkdir -p lib/rxi/microtar
	curl -sL https://github.com/rxi/microtar/archive/refs/heads/master.tar.gz | tar xzv --strip-components=1 -C lib/rxi/microtar
	mkdir -p lib/.dep/include/rxi
	ln -s ../../../rxi/microtar/src/microtar.h lib/.dep/include/rxi/microtar.h

lib/tidwall/json.c:
	mkdir -p lib/tidwall/json.c
	curl -sL https://github.com/tidwall/json.c/archive/refs/heads/main.tar.gz | tar xzv --strip-components=1 -C lib/tidwall/json.c
	mkdir -p lib/.dep/include/tidwall
	ln -s ../../../tidwall/json.c/json.h lib/.dep/include/tidwall/json.h

.c.o:
	${CC} $< ${CFLAGS} -c -o $@

dep: $(LIBS) $(OBJ)
	${CC} ${OBJ} ${CFLAGS} ${LDFLAGS} -o $@
	strip --strip-all $@

.PHONY: install
install: dep
	install dep ${DESTDIR}/bin

.PHONY: clean
clean:
	rm -f $(OBJ)

.PHONY: format
format:
	$(FIND) src/ -type f \( -name '*.c' -o -name '*.h' \) -exec clang-format -i {} +
