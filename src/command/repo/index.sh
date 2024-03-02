# #include "util/ini.sh"

read -r -d '' help_topics[repository] <<- EOF
# #include "help.txt"
EOF

CMD_REPO_CMD=
CMD_REPO_NAME=
CMD_REPO_LOC=

function arg_r {
  arg_repository "$@"
  return $?
}
function arg_repo {
  arg_repository "$@"
  return $?
}
function arg_repository {
  CMD_REPO_CMD=$1

  case "${CMD_REPO_CMD}" in
    a|add)
      CMD_REPO_NAME=$2
      CMD_REPO_LOC=$3
      ;;
    d|del|delete)
      CMD_REPO_NAME=$2
      ;;
    c|clean)
      # Intentionally empty
      ;;
    u|update)
      # Intentionally empty
      ;;
    *)
      echo "Unknown command: ${CMD_REPO_CMD}" >&2
      exit 1
      ;;
  esac

  return 0
}

function cmd_r {
  cmd_repository "$@"
  return $?
}
function cmd_repo {
  cmd_repository "$@"
  return $?
}
function cmd_repository {
  case "${CMD_REPO_CMD}" in
    a|add)
      mkdir -p "${HOME}/.config/finwo/__NAME/repositories.d"
      echo "${CMD_REPO_NAME}=${CMD_REPO_LOC}" >> "${HOME}/.config/finwo/__NAME/repositories.d/50-${CMD_REPO_NAME}"
      ;;
    d|del|delete)
      mkdir -p "${HOME}/.config/finwo/__NAME/repositories.d"
      rm -f "${HOME}/.config/finwo/__NAME/repositories.d/*-${CMD_REPO_NAME}"
      ;;
    c|clean)
      rm -rf "${HOME}/.config/finwo/__NAME/packages"
      mkdir -p "${HOME}/.config/finwo/__NAME/packages"
      ;;
    u|update)
      rm -rf "${HOME}/.config/finwo/__NAME/packages"
      mkdir -p "${HOME}/.config/finwo/__NAME/packages"
      mkdir -p "${HOME}/.config/finwo/__NAME/repositories.d"

      # Build complete repositories ini
      echo "" > "${HOME}/.config/finwo/__NAME/repositories.tmp"
      if [ -f "${HOME}/.config/finwo/__NAME/repositories" ]; then
        echo "[repository]" >> "${HOME}/.config/finwo/__NAME/repositories.tmp"
        cat "${HOME}/.config/finwo/__NAME/repositories" >> "${HOME}/.config/finwo/__NAME/repositories.tmp"
      fi
      for fname in $(ls "${HOME}/.config/finwo/__NAME/repositories.d/" | sort); do
        echo "[repository]" >> "${HOME}/.config/finwo/__NAME/repositories.tmp"
        cat "${HOME}/.config/finwo/__NAME/repositories.d/${fname}" >> "${HOME}/.config/finwo/__NAME/repositories.tmp"
      done

      # Download and extract them
      while read source; do
        curl --location --progress-bar "${source}" | \
          tar --gunzip --extract --directory "${HOME}/.config/finwo/__NAME/packages" --strip-components 1
      done < <(ini_foreach ini_output_value "${HOME}/.config/finwo/__NAME/repositories.tmp" "repository.")

      # Aannddd.. we're done with the tmp file
      rm -f "${HOME}/.config/finwo/__NAME/repositories.tmp"

      ;;
    *)
      echo "Unknown command: ${CMD_REPO_CMD}" >&2
      exit 1
      ;;
  esac
}

cmds[${#cmds[*]}]="r"
cmds[${#cmds[*]}]="repo"
cmds[${#cmds[*]}]="repository"
