declare -A help_topics

read -r -d '' help_topics[global] <<- EOF
# #include "topic/global.txt"
EOF

HELP_TOPIC=global
function arg_h {
  arg_help "$@"
  return $?
}
function arg_help {
  if [[ $# -gt 0 ]]; then
    HELP_TOPIC=$1
  fi
  shift
}

function cmd_h {
  cmd_help "$@"
  return $?
}
function cmd_help {
  if [ -z "${help_topics[$HELP_TOPIC]}" ]; then
    echo "Unknown topic: $HELP_TOPIC" >&2
    exit 1
  fi

  echo -e "\n${help_topics[$HELP_TOPIC]}\n"
}

cmds[${#cmds[*]}]="h"
cmds[${#cmds[*]}]="help"
