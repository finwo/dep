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
  HELP_TOPIC=$1
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

  echo "${help_topics[$HELP_TOPIC]}"
}

cmds[${#cmds[*]}]="h"
cmds[${#cmds[*]}]="help"
