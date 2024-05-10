# #include "util/ini.sh"
# #include "util/ostype.sh"

read -r -d '' help_topics[install] <<- EOF
# #include "help.txt"
EOF

CMD_INSTALL_PKG_NAME=
CMD_INSTALL_PKG_DEST="lib"

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

  # Fetch where to install dependencies
  CMD_INSTALL_PKG_DEST=$(ini_foreach ini_output_value "${PACKAGE_PATH}" "package.deps")
  if [ -z "${CMD_INSTALL_PKG_DEST}" ]; then CMD_INSTALL_PKG_DEST="lib"; fi

  # Reset working directory - keep cache
  rm -fr "${CMD_INSTALL_PKG_DEST}/.__NAME/include"
  mkdir -p "${CMD_INSTALL_PKG_DEST}/.__NAME/include"
  echo "INCLUDES+=-I${CMD_INSTALL_PKG_DEST}/.__NAME/include" > "${CMD_INSTALL_PKG_DEST}/.__NAME/config.mk"
  echo -n "" > "${CMD_INSTALL_PKG_DEST}/.__NAME/exported"

  # Install all dependencies
  ini_foreach cmd_install_parse_ini "${PACKAGE_PATH}"
  echo "Done"
}

cmds[${#cmds[*]}]="i"
cmds[${#cmds[*]}]="install"

declare -A CMD_INSTALL_DEPS
function cmd_install_parse_ini {
  case "$1" in
    dependencies.)
      cmd_install_dep "$2" "$3"
      ;;
  esac

    # package.)
    #   case "$2" in
    #     name)
    #       CMD_INSTALL_PKG_NAME="$3"
    #       ;;
    #     deps)
    #       CMD_INSTALL_PKG_DEST="$3"
    #       ;;
    #   esac
    #   ;;
    # dependencies.)
    #   CMD_INSTALL_DEPS["$2"]="$3"
    #   ;;
  # esac

}

# function cmd_install_execute {
#   cmd_install_reset_generated
#   for key in "${!CMD_INSTALL_DEPS[@]}"; do
#     cmd_install_dep "$key" "${CMD_INSTALL_DEPS[$key]}"
#   done
# }

function cmd_install_dep {
  local PKGNAME=$1
  local PKGVER=$2

  # Fetch versioned ini
  local PKGINIB="${HOME}/.config/finwo/__NAME/packages/${PKGNAME}/package.ini"
  local PKGINIV="${HOME}/.config/finwo/__NAME/packages/${PKGNAME}/${PKGVER}/package.ini"
  local PKGINI=
  if [ -f "${PKGINIB}" ]; then PKGINI="${PKGINIB}"; fi
  if [ -f "${PKGINIV}" ]; then PKGINI="${PKGINIV}"; fi
  if [ -z "${PKGINI}" ]; then
    echo "No package configuration found for ${PKGNAME}" >&2
    exit 1
  fi

  local PKG_DIR="${CMD_INSTALL_PKG_DEST}/${PKGNAME}"
  if [ -d "${PKG_DIR}" ]; then
    # Already installed, just update the pkgini ref
    PKGINI="${CMD_INSTALL_PKG_DEST}/${PKGNAME}/package.ini"
  else
    # Not installed yet, fetch code & run build steps

    # Copy repository's config for the package
    local PKG_SRC=$(dirname "${PKGINI}")
    mkdir -p "$(dirname "${PKG_DIR}")"
    cp -r "${PKG_SRC}" "$(dirname ${PKG_DIR})"
    PKGINI="${PKG_DIR}/package.ini"

    # Extended fetching detection
    local PKG_GH=$(ini_foreach ini_output_value "${PKGINI}" "repository.github")
    local PKG_TARBALL=$(ini_foreach ini_output_value "${PKGINI}" "package.src")

    # Fetch target tarball from github repo
    if [ ! -z "${PKG_GH}" ]; then
      URL_TAG="https://codeload.github.com/${PKG_GH}/tar.gz/refs/tags/${PKGVER}"
      URL_BRANCH="https://codeload.github.com/${PKG_GH}/tar.gz/refs/heads/${PKGVER}"
      CODE_TAG=$(curl -X HEAD --fail --dump-header - -o /dev/null "${URL_TAG}" 2>/dev/null | head -1 | awk '{print $2}')
      CODE_BRANCH=$(curl -X HEAD --fail --dump-header - -o /dev/null "${URL_BRANCH}" 2>/dev/null | head -1 | awk '{print $2}')
      if [ "${CODE_TAG}" == "200" ]; then
        PKG_TARBALL="${URL_TAG}"
      elif [ "${CODE_BRANCH}" == "200" ]; then
        PKG_TARBALL="${URL_BRANCH}"
      fi
    fi

    # Fetch configured or detected tarball
    if [ ! -z "${PKG_TARBALL}" ]; then
      # Downloads a tarball and extracts if over the package in our dependency directory
      # TARBALL_FILE="${HOME}/.config/finwo/__NAME/cache/${PKGNAME}/${PKGVER}.tar.gz"
      TARBALL_FILE="${CMD_INSTALL_PKG_DEST}/.__NAME/cache/${PKGNAME}/${PKGVER}.tar.gz"
      mkdir -p "$(dirname "${TARBALL_FILE}")"
      if [ ! -f "${TARBALL_FILE}" ]; then
        curl --location --progress-bar "${PKG_TARBALL}" --output "${TARBALL_FILE}"
      fi
      tar --extract --directory "${PKG_DIR}/" --strip-components 1 --file="${TARBALL_FILE}"
    fi

    # Install this dependency's dependencies
    while read line; do
      depname=${line%%=*}
      depver=${line#*=}
      cmd_install_dep "$depname" "$depver"
    done < <(ini_foreach ini_output_section "${PKGINI}" "dependencies." | sort --human-numeric-sort)

    # Handle any global build-steps defined in the package.ini
    while read line; do
      buildcmd=${line#*=}
      echo + $buildcmd
      bash -c "cd '${PKG_DIR}' ; ${buildcmd}"
    done < <(ini_foreach ini_output_section "${PKGINI}" "build." | sort --human-numeric-sort)

    # Handle any os-generic build-steps defined in the package.ini
    while read line; do
      buildcmd=${line#*=}
      echo + $buildcmd
      bash -c "cd '${PKG_DIR}' ; ${buildcmd}"
    done < <(ini_foreach ini_output_section "${PKGINI}" "build-$(ostype)." | sort --human-numeric-sort)
  fi

  # Build the package's exports
  if ! grep "${PKGNAME}" "${CMD_INSTALL_PKG_DEST}/.__NAME/exported" &>/dev/null ; then
    echo "${PKGNAME}" >> "${CMD_INSTALL_PKG_DEST}/.__NAME/exported"
    while read line; do
      filetarget=${line%%=*}
      filesource=${line#*=}
      mkdir -p "$(dirname "${CMD_INSTALL_PKG_DEST}/.__NAME/${filetarget}")"
      case "${filetarget}" in
        exported|cache/*)
          # Blocked
          ;;
        config.mk)
          cat "${PKG_DIR}/${filesource}" | sed "s|__DIRNAME|${PKG_DIR}|g" >> "${CMD_INSTALL_PKG_DEST}/.__NAME/${filetarget}"
          ;;
        *)
          # ls -sf "${CMD_INSTALL_PKG_DEST}/${name}/${filesource}" "${CMD_INSTALL_PKG_DEST}/.__NAME/${filetarget}"
          cp "${PKG_DIR}/${filesource}" "${CMD_INSTALL_PKG_DEST}/.__NAME/${filetarget}"
          ;;
      esac
    done < <(ini_foreach ini_output_section "${PKGINI}" "export.")
  fi

}
