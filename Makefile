SRC=$(wildcard src/*.sh)

# Idea:
# include lib/.dep/libraries.mk

PREPROCESS=preprocess --substitute

default: main

util/ini.sh: src/util/ini.sh src/util/shopt.sh
	mkdir -p util
	echo '#!/usr/bin/env bash' > $@
	$(PREPROCESS) -I src -v src/util/ini.sh | tee -a $@ > /dev/null
	chmod +x $@

.PHONY: main
main: $(SRC) util/ini.sh
	@$(eval NAME=$(shell util/ini.sh package.ini package.name))
	echo '#!/usr/bin/env bash' > "$(NAME)"
	$(PREPROCESS) -D __NAME=$(NAME) -I src src/main.sh | tee -a $(NAME) > /dev/null
	chmod +x "$(NAME)"

.PHONY: clean
clean:
	@$(eval NAME=$(shell util/ini.sh package.ini package.name))
	rm -f $(NAME)
	rm -rf util/ini.sh
