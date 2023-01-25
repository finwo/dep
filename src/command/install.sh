# #include "util/ini.sh"

function arg_i {
  arg_install "$@"
  return $?
}
function arg_install {
  return 0
}

function cmd_i {
  cmd_install "$@"
  return $?
}
function cmd_install {

  # Sanity check
  PACKAGE_PATH="$(pwd)/package.ini"
  if [ ! -f "${PACKAGE_PATH}" ]; then
    echo "No package.ini in the working directory!" >&2
    exit 1
  fi

  ini_foreach cmd_install_parse_ini "${PACKAGE_PATH}"
  cmd_install_execute
}

cmds[${#cmds[*]}]="i"
cmds[${#cmds[*]}]="install"

CMD_INSTALL_PKG_NAME=
CMD_INSTALL_PKG_DEST="$(pwd)/lib"
declare -A CMD_INSTALL_DEPS
function cmd_install_parse_ini {
  case "$1" in
    package.)
      case "$2" in
        name)
          CMD_INSTALL_PKG_NAME="$3"
          ;;
        deps)
          CMD_INSTALL_PKG_DEST="$(pwd)/$3"
          ;;
      esac
      ;;
    dependencies.)
      CMD_INSTALL_DEPS["$2"]="$3"
      ;;
  esac
}

function cmd_install_execute {
  echo "PKG_NAME: $CMD_INSTALL_PKG_NAME"
  echo "LIB_DEST: $CMD_INSTALL_PKG_DEST"
  echo "@: ${CMD_INSTALL_DEPS[@]}"
  echo "!: ${!CMD_INSTALL_DEPS[@]}"
}
