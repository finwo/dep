# #include "util/ini.sh"

read -r -d '' help_topics[add] <<- EOF
# #include "help.txt"
EOF

CMD_ADD_ARGS=

function arg_a {
  arg_add "$@"
  return $?
}
function arg_add {
  CMD_ADD_ARGS=("$@")

  # # Check if name exists in the repositories
  # # Returns if found
  # mkdir -p "${HOME}/.config/finwo/dep/repositories.d"
  # while read repo; do

  #   # Trim whitespace
  #   repo=${repo##*( )}
  #   repo=${repo%%*( )}

  #   # Remove comments and empty lines
  #   if [[ "${repo:0:1}" == '#' ]] || [[ "${repo:0:1}" == ';' ]] || [[ "${#repo}" == 0 ]]; then
  #     continue
  #   fi

  #   while read line; do
  #     pkgname="${line%%=*}"
  #     pkgloc="${line##*=}"
  #     # If found, return it
  #     if [ "${pkgname}" == "$1" ]; then
  #       CMD_ADD_SRC="${pkgloc}"
  #       break 2
  #     fi
  #   done < <(curl --location --silent "${repo}")
  # done < <(find "${HOME}/.config/finwo/dep/repositories.d" -type f -name '*.cnf' | xargs -n 1 -P 1 cat)

  # # Need 2 arguments from here on out
  # if [ -z "${CMD_ADD_SRC}" ] && [ $# != 2 ]; then
  #   echo "Add command requires 2 arguments" >&2
  #   exit 1
  # fi

  # CMD_ADD_PKG="$1"
  # if [ -z "${CMD_ADD_SRC}" ]; then
  #   CMD_ADD_SRC="$2"
  # fi

  return 0
}

function cmd_a {
  cmd_add "$@"
  return $?
}
function cmd_add {
  OLD_PKG=$(ini_foreach ini_output_full "package.ini")

  # TODO: Assume package name is github repo if missing?

  # Get target & main package file
  PKG=(${CMD_ADD_ARGS[0]//@/ })
  PKGINIB="${HOME}/.config/finwo/__NAME/packages/${PKG[0]}/package.ini"
  if [ ! -f "${PKGINIB}" ]; then
    echo "Package not found. Did you update your repositories?" >&2
    exit 1
  fi

  # Get the version to add
  if [ -z "${PKG[1]}" ]; then PKG[1]=$(ini_foreach ini_output_value "${PKGINIB}" "package.channel" | tail -n 1); fi
  if [ -z "${PKG[1]}" ]; then PKG[1]=$(ini_foreach ini_output_value "package.ini" "package.channel" | tail -n 1); fi
  if [ -z "${PKG[1]}" ]; then
    echo "Could not determine desired package version. Set 'package.channel' in your package.ini to select a fallback or use 'dep repository add ${PKG[0]}@<version>' to select a specific version." >&2
    exit 1
  fi

  # Fetch the version-specific ini
  PKGINIV="${HOME}/.config/finwo/__NAME/packages/${PKG[0]}/${PKG[1]}/package.ini"
  PKGINI=
  if [ -f "${PKGINIB}" ]; then PKGINI="${PKGINIB}"; fi
  if [ -f "${PKGINIV}" ]; then PKGINI="${PKGINIV}"; fi

  # Extension: Check release/branch on github
  PKGGH=$(ini_foreach ini_output_value "${PKGINI}" "repository.github")
  if [ ! -z "${PKGGH}" ]; then
    RELEASEURL="https://api.github.com/repos/${PKGGH}/releases/tags/${PKG[1]}"
    BRANCHURL="https://api.github.com/repos/${PKGGH}/branches/${PKG[1]}"
    CODE_RELEASE=$(curl --fail --dump-header - -o /dev/null "${RELEASEURL}" 2>/dev/null | head -1 | awk '{print $2}')
    CODE_BRANCH=$(curl --fail --dump-header - -o /dev/null "${BRANCHURL}" 2>/dev/null | head -1 | awk '{print $2}')
    if [ "${CODE_RELEASE}" != "200" ] && [ "${CODE_BRANCH}" != "200" ]; then
      echo "No release or branch '${PKG[1]}' found in the github repository ${PKGGH}." >&2
      echo "Check https://github.com/${PKGGH} to see the available releases and branches" >&2
      exit 1
    fi
  fi

  # Add the package to the dependencies
  (echo "dependencies.${PKG[0]}=${PKG[1]}" ; echo -e "${OLD_PKG}") | ini_write "package.ini"
  echo "Added to your package.ini: ${PKG[0]}@${PKG[1]}"
}

cmds[${#cmds[*]}]="a"
cmds[${#cmds[*]}]="add"
