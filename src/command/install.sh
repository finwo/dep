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

  ini_foreach cmd_install_parse_ini_main "${PACKAGE_PATH}"
  cmd_install_execute
}

cmds[${#cmds[*]}]="i"
cmds[${#cmds[*]}]="install"

CMD_INSTALL_PKG_NAME=
CMD_INSTALL_PKG_DEST="$(pwd)/lib"
declare -A CMD_INSTALL_DEPS
function cmd_install_parse_ini_main {
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
  cmd_install_reset_generated
  for key in "${!CMD_INSTALL_DEPS[@]}"; do
    cmd_install_dep "$key" "${CMD_INSTALL_DEPS[$key]}"
  done
}

function cmd_install_reset_generated {
  rm -rf "${CMD_INSTALL_PKG_DEST}/.__NAME"
  mkdir -p "${CMD_INSTALL_PKG_DEST}/.__NAME/include"
  echo "CFLAGS+=-I${CMD_INSTALL_PKG_DEST}/.__NAME/include" > "${CMD_INSTALL_PKG_DEST}/.__NAME/config.mk"
}

function cmd_install_dep_parse_package {
  echo PARSE PKG
}

function cmd_install_dep {
  name=$1
  origin=$2

  # Full install if missing
  if [ ! -d "${CMD_INSTALL_PKG_DEST}/${name}" ]; then

    # Fetch package.ini for the dependency
    mkdir -p "${CMD_INSTALL_PKG_DEST}/${name}"
    curl --location --progress-bar "${origin}" --output "${CMD_INSTALL_PKG_DEST}/${name}/package.ini"

    # Fetch it's src (if present)
    if [ ! -z "$(ini_foreach ini_output_value "${CMD_INSTALL_PKG_DEST}/${name}/package.ini" package.src)" ]; then
      SRC="$(ini_foreach ini_output_value "${CMD_INSTALL_PKG_DEST}/${name}/package.ini" package.src)"
      HASH="$(ini_foreach ini_output_value "${CMD_INSTALL_PKG_DEST}/${name}/package.ini" package.src-sha256)"

      # Download
      mkdir -p "${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${name}"
      if [ ! -f "${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${name}/tarball" ]; then
        curl --location --progress-bar "${SRC}" --output "${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${name}/tarball"
      fi

      # Verify checksum
      if [ "${HASH}" != "$(sha256sum "${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${name}/tarball" | awk '{print $1}')" ]; then
        echo "The tarball for '${name}' failed it's checksum!" >&2
        exit 1
      fi

      # Extract tarball
      tar --extract --directory "${CMD_INSTALL_PKG_DEST}/${NAME}/" --strip-components 1 --file="${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${name}/tarball"
    fi

    # Step 3: Execute fetch (for patches etc)
  else
    echo "ALREADY INSTALLED: ${name}"
  fi

  # Step 4: Build exports




  echo "Name       : $name"
  echo "Origin     : $origin"
  echo "Destination: $destination"
}





