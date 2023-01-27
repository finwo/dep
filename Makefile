SRC=$(wildcard src/*.sh)

# Idea:
# include lib/.dep/libraries.mk

PREPROCESS=preprocess --substitute
DESTDIR?=/usr/local
TARGET=dep

default: $(TARGET)

# util/ini.sh: src/util/ini.sh src/util/shopt.sh
# 	mkdir -p util
# 	echo '#!/usr/bin/env bash' > $@
# 	$(PREPROCESS) -I src -v src/util/ini.sh | tee -a $@ > /dev/null
# 	chmod +x $@

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
	# rm -rf util/ini.sh
