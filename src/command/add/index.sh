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
    URL_TAG="https://codeload.github.com/${PKGGH}/tar.gz/refs/tags/${PKG[1]}"
    URL_BRANCH="https://codeload.github.com/${PKGGH}/tar.gz/refs/heads/${PKG[1]}"
    CODE_TAG=$(curl -X HEAD --fail --dump-header - -o /dev/null "${URL_TAG}" 2>/dev/null | head -1 | awk '{print $2}')
    CODE_BRANCH=$(curl -X HEAD --fail --dump-header - -o /dev/null "${URL_BRANCH}" 2>/dev/null | head -1 | awk '{print $2}')
    if [ "${CODE_TAG}" != "200" ] && [ "${CODE_BRANCH}" != "200" ]; then
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
