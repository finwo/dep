SRC=$(wildcard src/*.sh)

# Idea:
# include lib/.dep/libraries.mk

default: main

bashpp:
	curl https://raw.githubusercontent.com/iwonbigbro/bashpp/master/bin/bashpp > bashpp
	chmod +x bashpp

util/ini.sh: src/ini.sh src/shopt.sh
	mkdir -p util
	echo '#!/usr/bin/env bash' > $@
	./bashpp -I src -v src/ini.sh | tee -a $@ > /dev/null
	chmod +x $@

.PHONY: main
main: $(SRC) bashpp util/ini.sh
	@$(eval NAME=$(shell util/ini.sh package.ini package.name))
	echo '#!/usr/bin/env bash' > "$(NAME)"
	./bashpp -D __NAME=$(NAME) -I src -v src/main.sh | tee -a "$(NAME)" > /dev/null
	chmod +x "$(NAME)"

.PHONY: clean
clean:
	@$(eval NAME=$(shell util/ini.sh package.ini package.name))
	rm -f $(NAME)
	rm -rf util/ini.sh
