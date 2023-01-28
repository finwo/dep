#include "util/ini.sh"

read -r -d '' help_topics[add] <<- EOF
# #include "help.txt"
EOF

CMD_ADD_PKG=
CMD_ADD_SRC=

function arg_a {
  arg_add "$@"
  return $?
}
function arg_add {

  # Check if name exists in the repositories
  # Returns if found
  mkdir -p "${HOME}/.config/finwo/dep/repositories.d"
  while read repo; do

    # Trim whitespace
    repo=${repo##*( )}
    repo=${repo%%*( )}

    # Remove comments and empty lines
    if [[ "${repo:0:1}" == '#' ]] || [[ "${repo:0:1}" == ';' ]] || [[ "${#repo}" == 0 ]]; then
      continue
    fi

    while read line; do
      pkgname="${line%%=*}"
      pkgloc="${line##*=}"
      # If found, return it
      if [ "${pkgname}" == "$1" ]; then
        CMD_ADD_SRC="${pkgloc}"
        break 2
      fi
    done < <(curl --location --silent "${repo}")
  done < <(find "${HOME}/.config/finwo/dep/repositories.d" -type f -name '*.cnf' | xargs -n 1 -P 1 cat)

  # Need 2 arguments from here on out
  if [ -z "${CMD_ADD_SRC}" ] && [ $# != 2 ]; then
    echo "Add command requires 2 arguments" >&2
    exit 1
  fi

  CMD_ADD_PKG="$1"
  if [ -z "${CMD_ADD_SRC}" ]; then
    CMD_ADD_SRC="$2"
  fi

  return 0
}

function cmd_a {
  cmd_add "$@"
  return $?
}
function cmd_add {
  OLD_PKG=$(ini_foreach ini_output_full "package.ini")
  (echo "dependencies.${CMD_ADD_PKG}=${CMD_ADD_SRC}" ; echo -e "${OLD_PKG}") | ini_write "package.ini"
}

cmds[${#cmds[*]}]="a"
cmds[${#cmds[*]}]="add"
