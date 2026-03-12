CC?=clang

FIND=$(shell which gfind find | head -1)
SRC:=
INCLUDES:=
CFLAGS:=
LDFLAGS:=

SRC+=$(shell $(FIND) src/ -type f -name '*.c')
INCLUDES+=-Isrc
INCLUDES+=-Ilib/.dep/include
LIBS:=

LIBS+=lib/cofyc/argparse
SRC+=lib/cofyc/argparse/argparse.c

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

OBJ:=$(SRC:.c=.o)
OBJ:=$(OBJ:.cc=.o)
CFLAGS+=${INCLUDES}

.PHONY: default
default: dep

lib/cofyc/argparse:
	mkdir -p lib/cofyc/argparse
	curl -sL https://github.com/cofyc/argparse/archive/refs/heads/master.tar.gz | tar xzv --strip-components=1 -C lib/cofyc/argparse
	mkdir -p lib/.dep/include/cofyc
	ln -s ../../../cofyc/argparse/argparse.h lib/.dep/include/cofyc/argparse.h

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

.c.o:
	${CC} $< ${CFLAGS} -c -o $@

dep: $(LIBS) $(OBJ)
	${CC} ${OBJ} ${CFLAGS} ${LDFLAGS} -o $@
