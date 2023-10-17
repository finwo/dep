SRC:=
SRC+=$(wildcard src/*.sh)
SRC+=$(wildcard src/*/*.sh)
SRC+=$(wildcard src/*/*/*.sh)

PREPROCESS=preprocess --substitute
DESTDIR?=/usr/local
TARGET=dep

default: dist/$(TARGET) README.md

dist/$(TARGET): $(SRC)
	mkdir -p $(shell dirname $@)
	echo '#!/usr/bin/env bash' > "$@"
	$(PREPROCESS) -D __NAME=$(TARGET) -I src src/main.sh | tee -a $@ > /dev/null
	chmod +x "$@"

.PHONY: install
install: dist/$(TARGET)
	install "dist/$(TARGET)" "$(DESTDIR)/bin"

README.md: README.md.html
	$(PREPROCESS) -D __NAME=$(TARGET) $< > "$@"

.PHONY: clean
clean:
	rm -rf dist
	rm -f README.md
