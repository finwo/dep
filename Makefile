SRC:=
SRC+=$(wildcard src/*.sh)
SRC+=$(wildcard src/*/*.sh)
SRC+=$(wildcard src/*/*/*.sh)

PREPROCESS=preprocess --substitute
DESTDIR?=/usr/local
TARGET=dep

dist/$(TARGET): $(SRC)
	mkdir -p $(shell dirname $@)
	echo '#!/usr/bin/env bash' > "$@"
	$(PREPROCESS) -D __NAME=$(TARGET) -I src src/main.sh | tee -a $@ > /dev/null
	chmod +x "$@"

.PHONY: install
install: dist/$(TARGET)
	install "dist/$(TARGET)" "$(DESTDIR)/bin"

.PHONY: clean
clean:
	rm -f $(TARGET)