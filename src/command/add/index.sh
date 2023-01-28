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
  if [ $# != 2 ]; then
    echo "Add command requires 2 arguments" >&2
    exit 1
  fi

  CMD_ADD_PKG="$1"
  CMD_ADD_SRC="$2"

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
