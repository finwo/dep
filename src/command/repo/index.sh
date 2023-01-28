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
  CMD_REPO_NAME=$2

  case "${CMD_REPO_CMD}" in
    add)
      CMD_REPO_LOC=$3
      ;;
    del)
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
    add)
      mkdir -p "${HOME}/.config/finwo/__NAME/repositories.d"
      echo "${CMD_REPO_LOC}" >> "${HOME}/.config/finwo/__NAME/repositories.d/${CMD_REPO_NAME}.cnf"
      ;;
    del)
      mkdir -p "${HOME}/.config/finwo/__NAME/repositories.d"
      rm -f "${HOME}/.config/finwo/__NAME/repositories.d/${CMD_REPO_NAME}.cnf"
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
