# #include "util/ini.sh"

read -r -d '' help_topics[install] <<- EOF
# #include "help.txt"
EOF

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
  echo "Done"
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
  echo "INCLUDES+=-I ${CMD_INSTALL_PKG_DEST}/.__NAME/include" > "${CMD_INSTALL_PKG_DEST}/.__NAME/config.mk"
}

function cmd_install_dep {
  local name=$1
  local origin=$2

  # Full install if missing
  local ISNEW=
  if [ ! -d "${CMD_INSTALL_PKG_DEST}/${name}" ]; then
    ISNEW="yes"

    # Fetch package.ini for the dependency
    mkdir -p "${CMD_INSTALL_PKG_DEST}/${name}"
    case "${origin##*.}" in
      ini)
        # Download the package.ini for the dependency
        curl --location --progress-bar "${origin}" --output "${CMD_INSTALL_PKG_DEST}/${name}/package.ini"
        ;;
      *)
        # Download the assumed tarball
        mkdir -p "${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${name}"
        if [ ! -f "${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${name}/tarball-pkg" ]; then
          curl --location --progress-bar "${origin}" --output "${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${name}/tarball-pkg"
        fi
        # Extract tarball
        tar --extract --directory "${CMD_INSTALL_PKG_DEST}/${name}/" --strip-components 1 --file="${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${name}/tarball-pkg"
        ;;
    esac

    # Fetch it's src (if present)
    SRC="$(ini_foreach ini_output_value "${CMD_INSTALL_PKG_DEST}/${name}/package.ini" package.src)"
    if [ ! -z "${SRC}" ]; then

      # Download
      mkdir -p "${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${name}"
      if [ ! -f "${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${name}/tarball-src" ]; then
        curl --location --progress-bar "${SRC}" --output "${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${name}/tarball-src"
      fi

      # Verify checksum
      HASH="$(ini_foreach ini_output_value "${CMD_INSTALL_PKG_DEST}/${name}/package.ini" package.src-sha256)"
      if [ ! -z "${HASH}" ] && [ "${HASH}" != "$(sha256sum "${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${name}/tarball-src" | awk '{print $1}')" ]; then
        echo "The tarball for '${name}' failed it's checksum!" >&2
        exit 1
      fi

      # Extract tarball
      tar --extract --directory "${CMD_INSTALL_PKG_DEST}/${name}/" --strip-components 1 --file="${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${name}/tarball-src"
    fi

    # Handle fetching extra files
    mkdir -p "${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${name}/fetch"
    while read line; do
      filename=${line%%=*}
      filesource=${line#*=}

      # Download the extra file into cache
      if [ ! -f "${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${name}/fetch/${filename}" ]; then
        curl --location --progress-bar "${filesource}" --create-dirs --output "${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${name}/fetch/${filename}"
      fi

    done < <(ini_foreach ini_output_section "${CMD_INSTALL_PKG_DEST}/${name}/package.ini" "fetch.")

    # Copy the extra files into the target directory
    tar --create --directory "${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${name}" "fetch" | \
      tar --extract --directory "${CMD_INSTALL_PKG_DEST}/${name}" --strip-components 1
  fi

  # Download package's dependencies
  while read line; do
    depname=${line%%=*}
    deplink=${line#*=}
    cmd_install_dep "$depname" "$deplink"
  done < <(ini_foreach ini_output_section "${CMD_INSTALL_PKG_DEST}/${name}/package.ini" "dependencies.")

  # Handle any build-steps defined in the package.ini
  if [ ! -z "$ISNEW" ]; then
    while read line; do
      buildcmd=${line#*=}
      bash -c "cd ${CMD_INSTALL_PKG_DEST}/${name} ; ${buildcmd}"
    done < <(ini_foreach ini_output_section "${CMD_INSTALL_PKG_DEST}/${name}/package.ini" "build." | sort --human-numeric-sort)
  fi

  # Build the package's exports
  while read line; do
    filetarget=${line%%=*}
    filesource=${line#*=}
    mkdir -p "$(dirname "${CMD_INSTALL_PKG_DEST}/.__NAME/${filetarget}")"
    case "${filetarget}" in
      config.mk)
        cat "${CMD_INSTALL_PKG_DEST}/${name}/${filesource}" | sed "s|__DIRNAME|${CMD_INSTALL_PKG_DEST}/${name}|g" >> "${CMD_INSTALL_PKG_DEST}/.__NAME/${filetarget}"
        ;;
      *)
        ln -fs "${CMD_INSTALL_PKG_DEST}/${name}/${filesource}" "${CMD_INSTALL_PKG_DEST}/.__NAME/${filetarget}"
        ;;
    esac
  done < <(ini_foreach ini_output_section "${CMD_INSTALL_PKG_DEST}/${name}/package.ini" "export.")
}
