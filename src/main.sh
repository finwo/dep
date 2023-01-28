cmds=("")
# #include "command/help/index.sh"
# #include "command/add/index.sh"
# #include "command/install/index.sh"
# #include "command/repo/index.sh"

function main {
  cmd=help

  while [ "$#" -gt 0 ]; do

    # If argument is a command, pass parsing on to it & stop main parser
    if [[ " ${cmds[*]} " =~ " $1 " ]]; then
      cmd=$1
      shift
      arg_$cmd "$@"
      break
    fi

    # Main parser
    case "$1" in
      --)
        shift
        break 2
        ;;
      *)
        echo "Unknown argument: $1" >&2
        exit 1
        ;;
    esac
    shift

  done

  cmd_$cmd
}

if [ $(basename $0) == "__NAME" ]; then
  main "$@"
fi
