SRC:=
SRC+=$(wildcard src/*.sh)
SRC+=$(wildcard src/*/*.sh)
SRC+=$(wildcard src/*/*/*.sh)

PREPROCESS=preprocess --substitute
DESTDIR?=/usr/local
TARGET=dep

$(TARGET): $(SRC)
	echo '#!/usr/bin/env bash' > "$(TARGET)"
	$(PREPROCESS) -D __NAME=$(TARGET) -I src src/main.sh | tee -a $(TARGET) > /dev/null
	chmod +x "$(TARGET)"

.PHONY: install
install: $(TARGET)
	install "$(TARGET)" "$(DESTDIR)/bin"

.PHONY: clean
clean:
	rm -f $(TARGET)
